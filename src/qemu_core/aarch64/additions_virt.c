#include "qemu/log.h"

static void create_virt_mulmatr_device(const VirtMachineState *vms, qemu_irq *pic)
{
    qemu_log_mask(CPU_LOG_MMU, "Inside function (virt.c) create_virt_mulmatr_device ######################### QEMU #########################\n");
    
    qemu_log_mask(CPU_LOG_MMU, "VIRT_MULMATR value %d\n", VIRT_MULMATR);
    qemu_log_mask(CPU_LOG_MMU, "mulmatr base (base_memmap) %"PRIx64"\n", base_memmap[VIRT_MULMATR].base);
    qemu_log_mask(CPU_LOG_MMU, "mulmatr base (ms->memmap) %"PRIx64"\n", vms->memmap[VIRT_MULMATR].base);
    
    //base_memmap[VIRT_MULMATR].base;

    hwaddr base = vms->memmap[VIRT_MULMATR].base;
    hwaddr size = vms->memmap[VIRT_MULMATR].size;
    int irq = vms->irqmap[VIRT_MULMATR];
    char *nodename;

    /*
     * virt-mulmatr@0b000000 {
     *         compatible = "virt-mulmatr";
     *         reg = <0x0b000000 0x500>;
     *         interrupt-parent = <&gic>;
     *         interrupts = <176>;
     * }
     */

    sysbus_create_simple("virt-mulmatr", base, pic[irq]);

    nodename = g_strdup_printf("/virt_mulmatr@%" PRIx64, base);

    qemu_log_mask(CPU_LOG_MMU, "HW_ADDR %" PRIx64, base);
    qemu_log_mask(CPU_LOG_MMU, " STR: %s\n", nodename);

    qemu_fdt_add_subnode(vms->fdt, nodename);
    qemu_fdt_setprop_string(vms->fdt, nodename, "compatible", "virt-mulmatr");
    qemu_fdt_setprop_sized_cells(vms->fdt, nodename, "reg", 2, base, 2, size);
    qemu_fdt_setprop_cells(vms->fdt, nodename, "interrupt-parent",
                           vms->gic_phandle);
    qemu_fdt_setprop_cells(vms->fdt, nodename, "interrupts",
                           GIC_FDT_IRQ_TYPE_SPI, irq,
                           GIC_FDT_IRQ_FLAGS_LEVEL_HI);

    g_free(nodename);
}