use crate::{arch::vm::HvArchvmConfig, config::*};

pub const ROOT_vm_DTB_ADDR: u64 = 0xa0000000;
pub const ROOT_vm_KERNEL_ADDR: u64 = 0xa0400000;
pub const ROOT_vm_ENTRY: u64 = 0xa0400000;
pub const ROOT_vm_CPUS: u64 = (1 << 0) | (1 << 1);

pub const ROOT_vm_NAME: &str = "root-linux";

pub const ROOT_vm_MEMORY_REGIONS: [HvConfigMemoryRegion; 3] = [
    HvConfigMemoryRegion {
        mem_type: MEM_TYPE_RAM,
        physical_start: 0x50000000,
        virtual_start: 0x50000000,
        size: 0x70000000,
    }, // ram
    HvConfigMemoryRegion {
        mem_type: MEM_TYPE_IO,
        physical_start: 0x9000000,
        virtual_start: 0x9000000,
        size: 0x1000,
    }, // serial
    HvConfigMemoryRegion {
        mem_type: MEM_TYPE_IO,
        physical_start: 0xa000000,
        virtual_start: 0xa000000,
        size: 0x4000,
    }, // virtio
];

// 35 36 37 38 -> pcie intx#
// 65 -> ivc
pub const ROOT_vm_IRQS: [u32; 9] = [33, 64, 77, 79, 35, 36, 37, 38, 65];

pub const ROOT_ARCH_vm_CONFIG: HvArchvmConfig = HvArchvmConfig {
    gicd_base: 0x8000000,
    gicd_size: 0x10000,
    gicr_base: 0x80a0000,
    gicr_size: 0xf60000,
    gicc_base: 0x8010000,
    gicc_size: 0x10000,
    gicc_offset: 0x0,
    gich_base: 0x8030000,
    gich_size: 0x10000,
    gicv_base: 0x8040000,
    gicv_size: 0x10000,
    gits_base: 0x8080000,
    gits_size: 0x20000,
};

pub const ROOT_PCI_CONFIG: HvPciConfig = HvPciConfig {
    ecam_base: 0x4010000000,
    ecam_size: 0x10000000,
    io_base: 0x3eff0000,
    io_size: 0x10000,
    pci_io_base: 0x0,
    mem32_base: 0x10000000,
    mem32_size: 0x2eff0000,
    pci_mem32_base: 0x10000000,
    mem64_base: 0x8000000000,
    mem64_size: 0x8000000000,
    pci_mem64_base: 0x8000000000,
};

pub const ROOT_vm_IVC_CONFIG: [HvIvcConfig; 0] = [];

pub const ROOT_PCI_DEVS: [u64; 2] = [0, 1 << 3];