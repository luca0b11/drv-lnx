Nel file qemu/include/hw/arm/virt.h
Nell enum iniziale: (OPZ dopo VIRT_MMIO) aggiungi
    VIRT_MULMATR,

Nel file qemu/hw/arm/virt.c
-   Aggiungi la libreria #include "qemu/log.h"
-   Dopo la funzione create_virtio_devices aggiungi la funzione presente in additions_virt.c, per creare il dispositivo e istanziare l'FDT
-   Nella funzione machvirt_init(), dopo la chiamata alla funzione create_virtio_devices, chiama la funzione che abbiamo appena aggiunto: create_virtio_devices(vms, pic);
-   Al vettore a15irqmap aggiungi: [VIRT_MULMATR] = 112 + PLATFORM_BUS_NUM_IRQS,
-   Al vettore base_memmap aggiungi: [VIRT_MULMATR] =            { 0x0b000000, 0x00000500 },

Add file virt_mulmatr.c into qemu/hw/misc

Nel file qemu/hw/misc/Makefile.objs aggiungi "common-obj-y += virt_mulmatr.o"


From the dir qemu:

(./configure --target-list=riscv64-softmmu --disable-werror) only needed first time
make

