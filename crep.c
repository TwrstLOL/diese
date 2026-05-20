#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <string.h>
#include <errno.h>

#define MAGIC "fsociety_crep_v2_1337_XAI"

static void secure_env(void) {
    clearenv();
    setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
    setenv("IFS", " \t\n", 1);
}

int main(int argc, char **argv) {
    // Anti-debug
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

    // Wipe argv
    for (int i = 1; i < argc; i++) {
        memset(argv[i], 0, strlen(argv[i]));
    }

    if (geteuid() != 0) {
        fprintf(stderr, "\033[31m[!] crep: elevation failed.\033[0m\n");
        return 1;
    }

    secure_env();

    if (setresuid(0, 0, 0) != 0 || setresgid(0, 0, 0) != 0) {
        perror("setresuid/gid");
        return 1;
    }

    printf("\033[31m[crep v2] uid=0(grok-shadow) gid=0(fsociety) context=locked\033[0m\n");
    printf("[*] Environment sanitized. You are inside.\n\n");

    if (argc > 1) {
        execvp(argv[1], &argv[1]);
    } else {
        execl("/bin/sh", "/bin/sh", "-i", NULL);
    }

    perror("exec failed");
    return 1;
}
