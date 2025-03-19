#ifndef __axvisor_H
#define __axvisor_H
#include <linux/ioctl.h>
#include <linux/types.h>

#include "def.h"
#include "vm_config.h"

#define MMAP_SIZE 4096
#define MAX_REQ 32
#define MAX_DEVS 4
#define MAX_CPUS 4
#define MAX_vmS MAX_CPUS

#define SIGHVI 10
// receive request from el2
struct device_req {
    __u64 src_cpu;
    __u64 address; // vm's ipa
    __u64 size;
    __u64 value;
    __u32 src_vm;
    __u8 is_write;
    __u8 need_interrupt;
    __u16 padding;
};

struct device_res {
    __u32 target_vm;
    __u32 irq_id;
};

struct virtio_bridge {
    __u32 req_front;
    __u32 req_rear;
    __u32 res_front;
    __u32 res_rear;
    struct device_req req_list[MAX_REQ];
    struct device_res res_list[MAX_REQ];
    __u64 cfg_flags[MAX_CPUS]; // avoid false sharing, set cfg_flag to u64
    __u64 cfg_values[MAX_CPUS];
    // TODO: When config is okay to use, remove these. It's ok to remove.
    __u64 mmio_addrs[MAX_DEVS];
    __u8 mmio_avail;
    __u8 need_wakeup;
};

struct ioctl_vm_list_args {
    __u64 cnt;
    vm_info_t *vms;
};

typedef struct ioctl_vm_list_args vm_list_args_t;

#define axvisor_INIT_VIRTIO _IO(1, 0) // virtio device init
#define axvisor_GET_TASK _IO(1, 1)
#define axvisor_FINISH_REQ _IO(1, 2) // finish one virtio req
#define axvisor_vm_START _IOW(1, 3, vm_config_t *)
#define axvisor_vm_SHUTDOWN _IOW(1, 4, __u64)
#define axvisor_vm_LIST _IOR(1, 5, vm_list_args_t *)

#define axvisor_HC_INIT_VIRTIO 0
#define axvisor_HC_FINISH_REQ 1
#define axvisor_HC_START_vm 2
#define axvisor_HC_SHUTDOWN_vm 3
#define axvisor_HC_vm_LIST 4

#ifdef LOONGARCH64

#define axvisor_CLEAR_INJECT_IRQ _IO(1, 6) // used for ioctl
#define axvisor_HC_CLEAR_INJECT_IRQ 20     // hvcall code in axvisor

#endif /* LOONGARCH64 */
#ifdef LOONGARCH64
static inline __u64 axvisor_call(__u64 code, __u64 arg0, __u64 arg1) {
    register __u64 a0 asm("a0") = code;
    register __u64 a1 asm("a1") = arg0;
    register __u64 a2 asm("a2") = arg1;
    // asm volatile ("hvcl"); // not supported by loongarch gcc now
    // hvcl 0 is 0x002b8000
    __asm__(".word 0x002b8000" : "+r"(a0), "+r"(a1), "+r"(a2));
    return a0;
}
#endif /* LOONGARCH64 */

#endif /* __axvisor_H */
