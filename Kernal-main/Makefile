obj-m += chardev.o

# Kernel build directory (adjust if needed)
KDIR := /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Load the module
load:
	sudo insmod chardev.ko
	sudo chmod 666 /dev/chardev

# Unload the module
unload:
	sudo rmmod chardev

# Show kernel messages
log:
	dmesg | tail -20

# Build user-space test application
test: test_chardev.c
	gcc -o test_chardev test_chardev.c -Wall

# Clean everything including test application
cleanall: clean
	rm -f test_chardev

.PHONY: all clean load unload log test cleanall
