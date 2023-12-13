Heavily inspired by the work from [liberpi](https://github.com/librerpi/lk).

Change Makefile /lib reference if you are not build on NixOS. 

# Interface
Uses a sysfs interface at `/sys/kernel/otp_interface`.

Two files:
- `in`: write a command
- `out`: read output

Commands are in the format: `r/w row_num [new_value]`
- `r` for read, `w` for write
- `row_num` for the OTP register to operate on
- `[new value]`, optional, to be supplied when writing (in hexadecimal)

All input/output format is hexadecimal