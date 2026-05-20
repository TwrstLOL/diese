CC = cc
CFLAGS = -static -O3 -s -fstack-protector-strong -fPIE -pie \
         -Wl,-z,relro,-z,now -D_FORTIFY_SOURCE=2

all: diese

diese: diese.c
	$(CC) $(CFLAGS) diese.c -o diese
	strip --strip-all diese 2>/dev/null || strip diese

clean:
	rm -f diese

.PHONY: all clean