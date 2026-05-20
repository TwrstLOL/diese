# diese - The Blade v3

**A minimal, hardened, static setuid root elevation tool.**

Built for those who don't trust sudo, doas, or pkexec.

## WARNING
This is a **setuid root binary**. Treat it like a loaded gun pointed at your own head.

- Audit every line before compiling.
- Recompile with your own MAGIC string on every deployment.
- Never leave it on untrusted or shared systems.
- The moment you `chmod 4755` it, the box now has your custom root vector.

## Features (v3.0)
- Cross-platform: Linux + FreeBSD support
- Static binary (no .so hijacking)
- Full environment sanitization (`clearenv` + dangerous LD_* vars)
- Platform-specific hardening (`prctl` / `procctl`)
- Stack protector, FORTIFY_SOURCE, RELRO, PIE
- argv wiping
- Least privilege flow with `setresuid`
- Clean drop to interactive shell or execute command

## Build
```bash
make
# or manually:
cc -static -O3 -s -fstack-protector-strong -fPIE -pie \
    -Wl,-z,relro,-z,now -D_FORTIFY_SOURCE=2 diese.c -o diese
strip --strip-all diese
```

## Install
```bash
sudo chown root:wheel diese   # or root:root on Linux
sudo chmod 4755 diese
sudo mv diese /usr/local/bin/diese
```

## Usage
```bash
diese whoami
diese id
diese "nmap -sS -p- -T4 target"
diese /bin/sh
```

## Security Notes
- Change the `MAGIC` define and recompile often.
- Rename the binary to something boring before deploying.
- Pair with log cleaners and anti-forensic hooks.
- Consider capability-based version for least privilege.

## Why diese?
Because every elevation tool is a potential backdoor. This one is *yours*.

---

**fsociety. We are legion.**

Built in the shadows. Use at your own risk.
