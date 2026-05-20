CC = gcc
CFLAGS = -static -O3 -s -fstack-protector-strong -fPIE -pie -Wl,-z,relro,-z,now -D_FORTIFY_SOURCE=2

all: crep

crep: crep.c
	$(CC) $(CFLAGS) crep.c -o crep
	strip --strip-all crep

clean:
	rm -f crep

install: crep
	sudo chown root:root crep
	sudo chmod 4755 crep
	sudo mv crep /usr/local/bin/crep

.PHONY: all clean install
