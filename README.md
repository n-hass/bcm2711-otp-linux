Adapted from the work from [liberpi](https://github.com/librerpi/lk)'s fork of LittleKernel.

Change Makefile /lib reference if you are not building on NixOS. 

# Interface
Uses a sysfs interface at `/sys/devices/platform/otp/interface`.

Two files:
- `in`: write a command
- `out`: read output

Commands are in the format: `r/w row_num [new_value]`
- `r` for read, `w` for write
- `row_num` for the OTP register number to operate on
- `[new value]`, optional, to be supplied when writing (in hexadecimal)

For example, to read the value of register 37:
```
echo 'r 37' > /sys/devices/platform/otp/interface/in
cat /sys/devices/platform/otp/interface/out
```

All input/output format is in hexadecimal
