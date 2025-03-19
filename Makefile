KDIR ?= 
DEV ?= /dev/sda1
ARCH ?= arm64
LOG ?= LOG_INFO
DEBUG ?= n
VIRTIO_GPU ?= n

export KDIR
export ARCH
export LOG
export VIRTIO_GPU
.PHONY: all env tools driver clean

# check if KDIR is set
ifeq ($(KDIR),)
$(error Linux kernel directory is not set. Please set environment variable 'KDIR')
endif

all: tools driver

env:
	git submodule update --init --recursive

tools: env
	$(MAKE) -C tools all

driver: env
	$(MAKE) -C driver all

transfer: all
	./trans_file.sh ./tools/axvisor 
	./trans_file.sh ./driver/axvisor.ko 

# transfer_nxp: all
# 	sudo mount $(DEV) /mnt/
# 	sudo rm -f /mnt/home/arm64/axvisor /mnt/home/arm64/axvisor.ko
# 	sudo cp ./tools/axvisor /mnt/home/arm64
# 	sudo cp ./driver/axvisor.ko /mnt/home/arm64
# 	sudo umount $(DEV)

transfer_nxp: all
	sudo cp ./tools/axvisor ~/tftp
	sudo cp ./tools/ivc_demo ~/tftp
	sudo cp ./tools/rpmsg_demo ~/tftp
	sudo cp ./driver/axvisor.ko ~/tftp
	sudo cp ./driver/ivc.ko ~/tftp

clean:
	make -C tools clean
	make -C driver clean