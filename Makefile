CFLAGS ?= -std=c99 -Wall -Wextra -pedantic
efi-boot-to-fw-ui : main.c Makefile
	$(CC) $(CFLAGS) -o $@ main.c
