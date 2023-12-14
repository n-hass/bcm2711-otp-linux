obj-m += bcm2711_otp.o
bcm2711_otp-objs := otp.o module.o

# KDIR := $(shell nix-build '<nixpkgs>' -A linuxPackages_latest.kernel.dev --no-out-link)/lib/modules/6.6.5/build

EXTRA_CFLAGS += -D RPI4 -D BCM2711 -D WRITE_ENABLED

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean