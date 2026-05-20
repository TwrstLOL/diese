CC = cc
CFLAGS = -O3 -s -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2 -fvisibility=hidden
LDFLAGS = -pie -Wl,-z,relro,-z,now

all: diese

diese: diese.c
	$(CC) $(CFLAGS) $(LDFLAGS) diese.c -o diese
	strip diese 2>/dev/null || true
	@echo "[+] diese v3.5 ready"
	@echo "    sudo chown root:wheel diese && sudo chmod 4755 diese"

clean:
	rm -f diese
