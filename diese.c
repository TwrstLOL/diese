#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdarg.h>
#include <limits.h>

#define CONF_FILE     "/etc/diese.conf"
#define MAX_LINE      1024
#define MAX_ARGS      64
#define MAX_RULES     256

/* ── Rule structure ─────────────────────────────────────────── */
typedef struct {
    int     permit;          /* 1 = permit, 0 = deny              */
    int     nopass;          /* skip password prompt               */
    int     keepenv;         /* preserve environment               */
    char    identity[256];   /* user or :group                     */
    char    target[256];     /* run as this user (default: root)   */
    char    cmd[PATH_MAX];   /* allowed command (empty = any)      */
    char    args[MAX_ARGS][256]; /* allowed args (empty = any)     */
    int     nargs;
} Rule;

static Rule  rules[MAX_RULES];
static int   nrules = 0;

/* ── Logging ────────────────────────────────────────────────── */
static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsyslog(LOG_AUTH | LOG_CRIT, fmt, ap);
    va_end(ap);
    fprintf(stderr, "diese: permission denied\n"); /* never leak details */
    exit(1);
}

static void audit(int permit, const char *user, const char *target,
                  char *const argv[]) {
    char cmd[4096] = {0};
    for (int i = 0; argv[i]; i++) {
        strncat(cmd, argv[i], sizeof(cmd) - strlen(cmd) - 2);
        strncat(cmd, " ",     sizeof(cmd) - strlen(cmd) - 1);
    }
    syslog(LOG_AUTH | LOG_INFO, "%s: user=%s target=%s cmd=%s",
           permit ? "PERMIT" : "DENY", user, target, cmd);
}

/* ── Config parser ──────────────────────────────────────────── */
static void parse_conf(void) {
    /* Security: verify config file ownership and permissions */
    struct stat st;
    if (stat(CONF_FILE, &st) != 0)
        die("cannot stat " CONF_FILE ": %s", strerror(errno));
    if (st.st_uid != 0)
        die(CONF_FILE " must be owned by root");
    if (st.st_mode & (S_IWGRP | S_IWOTH))
        die(CONF_FILE " must not be group/world writable");

    FILE *f = fopen(CONF_FILE, "r");
    if (!f)
        die("cannot open " CONF_FILE);

    char line[MAX_LINE];
    int  lineno = 0;

    while (fgets(line, sizeof(line), f)) {
        lineno++;
        /* Strip comments and trailing newline */
        char *p = strchr(line, '#');
        if (p) *p = '\0';
        p = line + strlen(line) - 1;
        while (p >= line && (*p == '\n' || *p == '\r' || *p == ' '))
            *p-- = '\0';
        if (!*line) continue;

        if (nrules >= MAX_RULES)
            die("too many rules in " CONF_FILE);

        Rule *r = &rules[nrules];
        memset(r, 0, sizeof(*r));
        strcpy(r->target, "root"); /* default target */

        /* Tokenise */
        char *tok = strtok(line, " \t");
        if (!tok) continue;

        if      (strcmp(tok, "permit") == 0) r->permit = 1;
        else if (strcmp(tok, "deny")   == 0) r->permit = 0;
        else { fprintf(stderr, "diese: syntax error line %d\n", lineno); continue; }

        /* Options */
        while ((tok = strtok(NULL, " \t"))) {
            if      (strcmp(tok, "nopass")   == 0) r->nopass  = 1;
            else if (strcmp(tok, "keepenv")  == 0) r->keepenv = 1;
            else break; /* identity comes next */
        }

        if (!tok) { fprintf(stderr, "diese: missing identity line %d\n", lineno); continue; }
        strncpy(r->identity, tok, sizeof(r->identity) - 1);

        /* Optional: as <target> */
        tok = strtok(NULL, " \t");
        if (tok && strcmp(tok, "as") == 0) {
            tok = strtok(NULL, " \t");
            if (!tok) { fprintf(stderr, "diese: missing target line %d\n", lineno); continue; }
            strncpy(r->target, tok, sizeof(r->target) - 1);
            tok = strtok(NULL, " \t");
        }

        /* Optional: cmd <path> [args] */
        if (tok && strcmp(tok, "cmd") == 0) {
            tok = strtok(NULL, " \t");
            if (!tok) { fprintf(stderr, "diese: missing cmd line %d\n", lineno); continue; }
            strncpy(r->cmd, tok, sizeof(r->cmd) - 1);
            while ((tok = strtok(NULL, " \t")) && r->nargs < MAX_ARGS - 1)
                strncpy(r->args[r->nargs++], tok, 255);
        }

        nrules++;
    }
    fclose(f);
}

/* ── Match helpers ──────────────────────────────────────────── */
static int user_in_group(const char *user, const char *group_name) {
    struct group *gr = getgrnam(group_name);
    if (!gr) return 0;
    for (char **m = gr->gr_mem; *m; m++)
        if (strcmp(*m, user) == 0) return 1;
    return 0;
}

static int identity_matches(const char *identity, const char *user) {
    if (identity[0] == ':')          /* group */
        return user_in_group(user, identity + 1);
    return strcmp(identity, user) == 0;
}

static int cmd_matches(const Rule *r, const char *cmd,
                       char *const argv[]) {
    if (!r->cmd[0]) return 1;        /* no cmd restriction */
    if (strcmp(r->cmd, cmd) != 0) return 0;
    if (r->nargs == 0) return 1;     /* no arg restriction */
    for (int i = 0; i < r->nargs; i++) {
        if (!argv[i + 1]) return 0;
        if (strcmp(r->args[i], argv[i + 1]) != 0) return 0;
    }
    return 1;
}

/* ── Privilege drop / exec ──────────────────────────────────── */
static void safe_exec(const char *target_user, int keepenv,
                      char *const argv[]) {
    struct passwd *pw = getpwnam(target_user);
    if (!pw) die("unknown target user: %s", target_user);

    /* Drop supplementary groups first */
    if (initgroups(pw->pw_name, pw->pw_gid) != 0)
        die("initgroups: %s", strerror(errno));
    if (setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) != 0)
        die("setresgid: %s", strerror(errno));
    if (setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) != 0)
        die("setresuid: %s", strerror(errno));

    /* Sanitise environment unless keepenv */
    if (!keepenv) {
        char *safe_env[] = {
            "PATH=/usr/local/bin:/usr/bin:/bin",
            NULL
        };
        execve(argv[0], argv, safe_env);
    } else {
        execv(argv[0], argv);
    }
    die("execv %s: %s", argv[0], strerror(errno));
}

/* ── main ───────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    openlog("diese", LOG_PID | LOG_NDELAY, LOG_AUTH);

    if (argc < 2) {
        fprintf(stderr, "usage: diese [-u user] command [args...]\n");
        return 1;
    }

    /* Must be setuid root */
    if (geteuid() != 0)
        die("diese must be installed setuid root");

    /* Get calling user */
    struct passwd *caller = getpwuid(getuid());
    if (!caller) die("cannot determine calling user");
    const char *caller_name = caller->pw_name;

    /* Parse -u <target> flag */
    char target[256] = "root";
    int  cmd_start   = 1;
    if (argc >= 3 && strcmp(argv[1], "-u") == 0) {
        strncpy(target, argv[2], sizeof(target) - 1);
        cmd_start = 3;
    }

    if (cmd_start >= argc)
        die("no command specified");

    /* Resolve absolute path of command */
    char cmd_path[PATH_MAX];
    if (realpath(argv[cmd_start], cmd_path) == NULL)
        die("cannot resolve %s: %s", argv[cmd_start], strerror(errno));

    parse_conf();

    /* Match rules (last matching rule wins, like doas) */
    Rule *matched = NULL;
    for (int i = 0; i < nrules; i++) {
        Rule *r = &rules[i];
        if (!identity_matches(r->identity, caller_name)) continue;
        if (strcmp(r->target, target) != 0) continue;
        if (!cmd_matches(r, cmd_path, &argv[cmd_start])) continue;
        matched = r;
    }

    audit(matched && matched->permit, caller_name, target, &argv[cmd_start]);

    if (!matched || !matched->permit)
        die("denied for %s", caller_name);

    /* TODO: add PAM/password prompt here when !matched->nopass */
    if (!matched->nopass) {
        fprintf(stderr, "diese: password auth not yet implemented — add PAM\n");
        /* Integrate libpam here for production use */
        exit(1);
    }

    /* Build exec argv */
    char **exec_argv = &argv[cmd_start];
    exec_argv[0]     = cmd_path;

    safe_exec(target, matched->keepenv, exec_argv);
    return 1; /* unreachable */
}
