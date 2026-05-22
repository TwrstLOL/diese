CC      = cc
CFLAGS  = -Wall -Wextra -Wpedantic -O2 -D_FORTIFY_SOURCE=2 \
           -fstack-protector-strong -fPIE -pie
LDFLAGS = -Wl,-z,relro,-z,now

diese: diese.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o diese diese.c
	chown root:root diese
	chmod 4755 diese       # setuid root

install: diese
	install -o root -g root -m 4755 diese /usr/local/bin/diese
	install -o root -g root -m 0640 diese.conf.example /etc/diese.conf
