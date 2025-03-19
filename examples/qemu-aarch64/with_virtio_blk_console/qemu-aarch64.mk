QEMU := qemu-system-aarch64

ifeq ($(findstring gicv3, $(FEATURES)),gicv3)
    UBOOT := $(image_dir)/bootloader/u-boot-atf.bin
    vm0_dtb := $(image_dir)/devicetree/linux1.dtb
    QEMU_ARGS := -machine virt,secure=on,gic-version=3,virtualization=on,iommu=smmuv3
    MESSAGE := "Note: Feature contains gicv3"
else
    UBOOT := $(image_dir)/bootloader/u-boot-v2.bin
    vm0_dtb := $(image_dir)/devicetree/linux1-v2.dtb
    QEMU_ARGS := -machine virt,secure=on,gic-version=2,virtualization=on,iommu=smmuv3
    MESSAGE := "Note: Feature contains gicv2"
endif

FSIMG1 := $(image_dir)/virtdisk/rootfs1.ext4
FSIMG2 := $(image_dir)/virtdisk/rootfs2.ext4

vm0_kernel := $(image_dir)/kernel/Image

QEMU_ARGS += -global arm-smmuv3.stage=2

QEMU_ARGS += -cpu cortex-a72
QEMU_ARGS += -smp 4
QEMU_ARGS += -m 2G
QEMU_ARGS += -nographic
QEMU_ARGS += -bios $(UBOOT)

QEMU_ARGS += -device loader,file="$(axvisor_bin)",addr=0x40400000,force-raw=on
QEMU_ARGS += -device loader,file="$(vm0_kernel)",addr=0xa0400000,force-raw=on
QEMU_ARGS += -device loader,file="$(vm0_dtb)",addr=0xa0000000,force-raw=on

QEMU_ARGS += -drive if=none,file=$(FSIMG1),id=Xa003e000,format=raw
QEMU_ARGS += -device virtio-blk-device,drive=Xa003e000,bus=virtio-mmio-bus.31

$(axvisor_bin): elf
	@if ! command -v mkimage > /dev/null; then \
		sudo apt update && sudo apt install u-boot-tools; \
	fi && \
	$(OBJCOPY) $(axvisor_elf) --strip-all -O binary $(axvisor_bin).tmp && \
	mkimage -n axvisor_img -A arm64 -O linux -C none -T kernel -a 0x40400000 \
	-e 0x40400000 -d $(axvisor_bin).tmp $(axvisor_bin) && \
	rm -rf $(axvisor_bin).tmp