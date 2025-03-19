#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
// #include <asm/io.h>
#include "axvisor.h"
#include "vm_config.h"
#include <asm/cacheflush.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>
#include <linux/sched/signal.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

struct virtio_bridge *virtio_bridge;
int virtio_irq = -1;
static struct task_struct *task = NULL;

// initial virtio el2 shared region
static int axvisor_init_virtio(void) {
    int err;
    if (virtio_irq == -1) {
        pr_err("virtio device is not available\n");
        return ENOTTY;
    }
    virtio_bridge = (struct virtio_bridge *)__get_free_pages(GFP_KERNEL, 0);
    if (virtio_bridge == NULL)
        return -ENOMEM;
    SetPageReserved(virt_to_page(virtio_bridge));
    // init device region
    memset(virtio_bridge, 0, sizeof(struct virtio_bridge));
    err = axvisor_call(axvisor_HC_INIT_VIRTIO, __pa(virtio_bridge), 0);
    if (err)
        return err;
    return 0;
}

// finish virtio req and send result to el2
static int axvisor_finish_req(void) {
    int err;
    err = axvisor_call(axvisor_HC_FINISH_REQ, 0, 0);
    if (err)
        return err;
    return 0;
}

// static int flush_cache(__u64 phys_start, __u64 size)
// {
//     struct vm_struct *vma;
//     int err = 0;
//     size = PAGE_ALIGN(size);
//     vma = __get_vm_area(size, VM_IOREMAP, VMALLOC_START, VMALLOC_END);
//     if (!vma)
//     {
//         pr_err("axvisor: failed to allocate virtual kernel memory for
//         image\n"); return -ENOMEM;
//     }
//     vma->phys_addr = phys_start;

//     if (ioremap_page_range((unsigned long)vma->addr, (unsigned
//     long)(vma->addr + size), phys_start, PAGE_KERNEL_EXEC))
//     {
//         pr_err("axvisor: failed to ioremap image\n");
//         err = -EFAULT;
//         goto unmap_vma;
//     }
//     // flush icache will also flush dcache
//     flush_icache_range((unsigned long)(vma->addr), (unsigned long)(vma->addr
//     + size));

// unmap_vma:
//     vunmap(vma->addr);
//     return err;
// }

static int axvisor_vm_start(vm_config_t __user *arg) {
    int err = 0;
    vm_config_t *vm_config = kmalloc(sizeof(vm_config_t), GFP_KERNEL);

    if (vm_config == NULL) {
        pr_err("axvisor: failed to allocate memory for vm_config\n");
    }

    if (copy_from_user(vm_config, arg, sizeof(vm_config_t))) {
        pr_err("axvisor: failed to copy from user\n");
        kfree(vm_config);
        return -EFAULT;
    }

    // flush_cache(vm_config->kernel_load_paddr, vm_config->kernel_size);
    // flush_cache(vm_config->dtb_load_paddr, vm_config->dtb_size);

    pr_info("axvisor: calling hypercall to start vm\n");

    err = axvisor_call(axvisor_HC_START_vm, __pa(vm_config),
                      sizeof(vm_config_t));
    kfree(vm_config);
    return err;
}

#ifndef LOONGARCH64
static int is_reserved_memory(unsigned long phys, unsigned long size) {
    struct device_node *parent, *child;
    struct reserved_mem *rmem;
    phys_addr_t mem_base;
    size_t mem_size;
    int count = 0;
    parent = of_find_node_by_path("/reserved-memory");
    count = of_get_child_count(parent);

    for_each_child_of_node(parent, child) {
        rmem = of_reserved_mem_lookup(child);
        mem_base = rmem->base;
        mem_size = rmem->size;
        if (mem_base <= phys && (mem_base + mem_size) >= (phys + size)) {
            return 1;
        }
    }
    return 0;
}
#endif

static int axvisor_vm_list(vm_list_args_t __user *arg) {
    int ret;
    vm_info_t *vms;
    vm_list_args_t args;

    /* Copy user provided arguments to kernel space */
    if (copy_from_user(&args, arg, sizeof(vm_list_args_t))) {
        pr_err("axvisor: failed to copy from user\n");
        return -EFAULT;
    }

    vms = kmalloc(args.cnt * sizeof(vm_info_t), GFP_KERNEL);
    memset(vms, 0, args.cnt * sizeof(vm_info_t));

    ret = axvisor_call(axvisor_HC_vm_LIST, __pa(vms), args.cnt);
    if (ret < 0) {
        pr_err("axvisor: failed to get vm list\n");
        goto out;
    }
    // copy result back to user space
    if (copy_to_user(args.vms, vms, ret * sizeof(vm_info_t))) {
        pr_err("axvisor: failed to copy to user\n");
        goto out;
    }
out:
    kfree(vms);
    return ret;
}

static long axvisor_ioctl(struct file *file, unsigned int ioctl,
                         unsigned long arg) {
    int err = 0;
    switch (ioctl) {
    case axvisor_INIT_VIRTIO:
        err = axvisor_init_virtio();
        task = get_current(); // get axvisor user process
        break;
    case axvisor_vm_START:
        err = axvisor_vm_start((vm_config_t __user *)arg);
        break;
    case axvisor_vm_SHUTDOWN:
        err = axvisor_call(axvisor_HC_SHUTDOWN_vm, arg, 0);
        break;
    case axvisor_vm_LIST:
        err = axvisor_vm_list((vm_list_args_t __user *)arg);
        break;
    case axvisor_FINISH_REQ:
        err = axvisor_finish_req();
        break;
#ifdef LOONGARCH64
    case axvisor_CLEAR_INJECT_IRQ:
        err = axvisor_call(axvisor_HC_CLEAR_INJECT_IRQ, 0, 0);
        break;
#endif
    default:
        err = -EINVAL;
        break;
    }
    return err;
}

// Kernel mmap handler
static int axvisor_map(struct file *filp, struct vm_area_struct *vma) {
    unsigned long phys;
    int err;
    if (vma->vm_pgoff == 0) {
        // virtio_bridge must be aligned to one page.
        phys = virt_to_phys(virtio_bridge);
        // vma->vm_flags |= (VM_IO | VM_LOCKED | (VM_DONTEXPAND | VM_DONTDUMP));
        // Not sure should we add this line.
        err = remap_pfn_range(vma, vma->vm_start, phys >> PAGE_SHIFT,
                              vma->vm_end - vma->vm_start, vma->vm_page_prot);
        if (err)
            return err;
        pr_info("virtio bridge mmap succeed!\n");
    } else {
        size_t size = vma->vm_end - vma->vm_start;
        // TODO: add check for non root memory region.
        // memremap(0x50000000, 0x30000000, MEMREMAP_WB);
        // vm_pgoff is the physical page number.
        // if (!is_reserved_memory(vma->vm_pgoff << PAGE_SHIFT, size)) {
        //     pr_err("The physical address to be mapped is not within the
        //     reserved memory\n"); return -EFAULT;
        // }
        err = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size,
                              vma->vm_page_prot);
        if (err)
            return err;
        pr_info("non root region mmap succeed!\n");
    }
    return 0;
}

static const struct file_operations axvisor_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = axvisor_ioctl,
    .compat_ioctl = axvisor_ioctl,
    .mmap = axvisor_map,
};

static struct miscdevice axvisor_misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "axvisor",
    .fops = &axvisor_fops,
};

// Interrupt handler for Virtio device.
static irqreturn_t virtio_irq_handler(int irq, void *dev_id) {
    struct siginfo info;
    if (dev_id != &axvisor_misc_dev) {
        return IRQ_NONE;
    }

    memset(&info, 0, sizeof(struct siginfo));
    info.si_signo = SIGHVI;
    info.si_code = SI_QUEUE;
    info.si_int = 1;
    // Send signal SIGHVI to axvisor user task
    if (task != NULL) {
        // pr_info("send signal to axvisor device\n");
        if (send_sig_info(SIGHVI, (struct kernel_siginfo *)&info, task) < 0) {
            pr_err("Unable to send signal\n");
        }
    }
    return IRQ_HANDLED;
}

/*
** Module Init function
*/
static int __init axvisor_init(void) {
    int err;
    struct device_node *node = NULL;
    err = misc_register(&axvisor_misc_dev);
    if (err) {
        pr_err("axvisor_misc_register failed!!!\n");
        return err;
    }
    // probe axvisor virtio device.
    // The irq number must be retrieved from dtb node, because it is different
    // from GIC's IRQ number.
    node = of_find_node_by_path("/axvisor_virtio_device");
    if (!node) {
        pr_info("axvisor_virtio_device node not found in dtb, can't use virtio "
                "devices\n");
    } else {
        virtio_irq = of_irq_get(node, 0);
        err = request_irq(virtio_irq, virtio_irq_handler,
                          IRQF_SHARED | IRQF_TRIGGER_RISING,
                          "axvisor_virtio_device", &axvisor_misc_dev);
        if (err)
            goto err_out;
    }
    of_node_put(node);
    pr_info("axvisor init done!!!\n");
    return 0;
err_out:
    pr_err("axvisor cannot register IRQ, err is %d\n", err);
    if (virtio_irq != -1)
        free_irq(virtio_irq, &axvisor_misc_dev);
    misc_deregister(&axvisor_misc_dev);
    return err;
}

/*
** Module Exit function
*/
static void __exit axvisor_exit(void) {
    if (virtio_irq != -1)
        free_irq(virtio_irq, &axvisor_misc_dev);
    if (virtio_bridge != NULL) {
        ClearPageReserved(virt_to_page(virtio_bridge));
        free_pages((unsigned long)virtio_bridge, 0);
    }
    misc_deregister(&axvisor_misc_dev);
    pr_info("axvisor exit!!!\n");
}

module_init(axvisor_init);
module_exit(axvisor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KouweiLee <15035660024@163.com>");
MODULE_DESCRIPTION("The axvisor device driver");
MODULE_VERSION("1:0.0");