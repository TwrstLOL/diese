# crep - The Blade

**A minimal, hardened, static setuid root elevation tool.**

Built for those who don't trust sudo, doas, or pkexec.

## WARNING
This is a **setuid root binary**. Treat it like a loaded gun pointed at your own head.
- Audit every line before compiling.
- Recompile with your own MAGIC string on every deployment.
- Never leave it on untrusted or shared systems.
- The moment you `chmod 4755` it, the box now has your custom root vector.

## Features (v2.0)
- Static binary (no .so hijacking)
- Full environment sanitization (`clearenv`)
- `PR_SET_NO_NEW_PRIVS` + no core dumps
- Stack protector, FORTIFY_SOURCE, RELRO, PIE
- argv wiping
- Least privilege flow with `setresuid`
- Clean drop to interactive shell or execute command

## Build
```bash
make
# or manually:
gcc -static -O3 -s -fstack-protector-strong -fPIE -pie \
    -Wl,-z,relro,-z,now -D_FORTIFY_SOURCE=2 crep.c -o crep
strip --strip-all crep
```

## Install
```bash
sudo chown root:root crep
sudo chmod 4755 crep
sudo mv crep /usr/local/bin/crep
```

## Usage
```bash
crep whoami
crep id
crep "nmap -sS -p- -T4 target"
crep /bin/sh
```

## Security Notes
- Change the `MAGIC` define and recompile often.
- Consider renaming the binary to something boring (`systemd-logger`, `klogd` etc.)
- Pair with log cleaners and anti-forensic hooks for real ops.
- Capabilities-based version coming in v3 (no full root).

## Why crep?
Because every elevation tool is a potential backdoor. This one is *yours*.

---

**fsociety. We are legion.**

Built in the shadows. Use at your own risk.
