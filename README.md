You can now get to the EFI firmware UI screen with some simple shell commands.
As root run:
```
$ printf "\x7\x0\x0\x0\x1\x0\x0\x0\x0\x0\x0\x0" > /sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c
$ reboot
```

This program uses a legacy 'efivars' sysfs interface.  This was removed in
Linux 6.0 so the program no longer works.
