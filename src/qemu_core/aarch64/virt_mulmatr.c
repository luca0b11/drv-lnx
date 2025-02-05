#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "qemu/bitops.h"
#include "qemu/log.h"

#define TYPE_VIRT_MULMATR          "virt-mulmatr"
#define VIRT_MULMATR(obj)          OBJECT_CHECK(VirtMulMatrState, (obj), TYPE_VIRT_MULMATR)

#define MATR_A_START        0x000
#define MATR_A_END	        0x190 	//0x000 + 100 * 4
#define MATR_B_START        0x200
#define MATR_B_END	        0x228 	//0x210 + 10 * 4
#define	MATR_C_START        0x300
#define	MATR_C_END	        0x328	//0x300 + 10 * 4

#define CONTROL_REG         0x400
#define BIT_C_ENABLE        BIT(0)
#define BIT_C_END_OP_IRQ_EN BIT(1)
#define BIT_C_START_OP      BIT(2)
#define BIT_C_RESET_STAT    BIT(3)  //also reset irq
#define DEFAULT_CTRL_REG    0x01

#define SIZE_REG            0x410
#define DEFAULT_SIZE_REG    0x03

#define STATUS_REG	        0x420
#define BIT_S_OP_STARTED    BIT(0)
#define BIT_S_OP_ENDED      BIT(1)

#define ID_REG              0x430
#define CHIP_ID             0xc1a0

#define MAX_SIZE            10
#define MAX_SIZE_QUAD       100

void matrix_vector_multiply(const int32_t *, const int32_t *, int32_t *, uint32_t);

typedef struct {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    qemu_irq irq;

    int32_t matrA[MAX_SIZE_QUAD];
	int32_t matrB[MAX_SIZE];

	int32_t matrC[MAX_SIZE];

	uint32_t control_reg;
	uint32_t size_reg;
	uint32_t status_reg;
    uint32_t id_reg;

} VirtMulMatrState;

void matrix_vector_multiply(const int32_t *matrix, const int32_t *vector, int32_t *result, uint32_t size)
{
    // Moltiplicazione matrice 'size x size' per vettore di 'size' elementi
    for (uint32_t row = 0; row < size; row++) {
		result[row] = 0;
        for (uint32_t col = 0; col < size; col++) {
            result[row] += matrix[row * size + col] * vector[col];
        }
    }
}

static uint64_t virt_mulmatr_read(void *opaque, hwaddr offset, unsigned size)
{
    qemu_log_mask(CPU_LOG_MMU, "QEMU: Inside function (virt_mulmatr.c) virt_mulmatr_read. Reading on addr 0x%x\n", (uint32_t)offset);

    VirtMulMatrState *s = (VirtMulMatrState *)opaque;
    bool is_enabled = s->control_reg & BIT_C_ENABLE;

    if (!is_enabled) {
        qemu_log_mask(CPU_LOG_MMU, "Device is disabled\n");
        return 0;
    }

    if((int)offset >= MATR_A_START && (int)offset <= MATR_A_END && ((int)offset%4 == 0))
	{
		return s->matrA[((int)offset-MATR_A_START)/4];
	} else if((int)offset >= MATR_B_START && (int)offset <= MATR_B_END && ((int)offset%4 == 0))
	{
		return s->matrB[((int)offset-MATR_B_START)/4];
	} else if((int)offset >= MATR_C_START && (int)offset <= MATR_C_END && ((int)offset%4 == 0))
	{
		return s->matrC[((int)offset-MATR_C_START)/4];
	} else if((int)offset == CONTROL_REG)
	{
		return s->control_reg;
	} else if((int)offset == SIZE_REG)
	{
		return s->size_reg;
	}else if((int)offset == STATUS_REG)
	{
        qemu_set_irq(s->irq, 0);
		return s->status_reg;
	}else if((int)offset == ID_REG)
	{
		return s->id_reg;
	} else return 0xA0E0A0E0;

    return 0;
}

static void virt_mulmatr_write(void *opaque, hwaddr offset, uint64_t data,
                          unsigned size)
{   
    qemu_log_mask(CPU_LOG_MMU, "QEMU: Inside function (virt_mulmatr.c) virt_mulmatr_write. Writinng 0x%x on addr 0x%x\n",data, (uint32_t)offset);

    VirtMulMatrState *s = (VirtMulMatrState *)opaque;

    if((int)offset >= MATR_A_START && (int)offset <= MATR_A_END && ((int)offset%4 == 0))
	{
		s->matrA[((int)offset-MATR_A_START)/4] = (int32_t)data;
	} else if((int)offset >= MATR_B_START && (int)offset <= MATR_B_END && ((int)offset%4 == 0))
	{
		s->matrB[((int)offset-MATR_B_START)/4] = (int32_t)data;
	}else if((int)offset == SIZE_REG)
	{
		s->size_reg = (data <= 10) ? (uint32_t)data : 10;
	}else if((int)offset == CONTROL_REG)
	{
		s->control_reg = data;

		if(data & BIT_C_START_OP)
		{	//Start the operation
			s->status_reg 	 |= BIT_S_OP_STARTED; //bit 0 = 1 Operation In progress
			matrix_vector_multiply((int32_t *)s->matrA, (int32_t *)s->matrB, (int32_t *)s->matrC, (uint32_t)s->size_reg);
			s->status_reg    |= BIT_S_OP_ENDED; //bit 1 = 1 Operation Ended
            
            if(s->control_reg & BIT_C_END_OP_IRQ_EN)
                qemu_set_irq(s->irq, 1);

		} else if(data & BIT_C_RESET_STAT)
		{	//Reset status Reg
			s->status_reg = 0x0;
            qemu_set_irq(s->irq, 0);
		}
	}
}

static const MemoryRegionOps virt_mulmatr_ops = {
    .read = virt_mulmatr_read,
    .write = virt_mulmatr_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void virt_mulmatr_realize(DeviceState *d, Error **errp)
{
    qemu_log_mask(CPU_LOG_MMU, "Inside function (virt_mulmatr.c) virt_mulmatr_realize ######################### QEMU #########################\n");

    VirtMulMatrState *s = VIRT_MULMATR(d);
    SysBusDevice *sbd = SYS_BUS_DEVICE(d);

    memory_region_init_io(&s->iomem, OBJECT(s), &virt_mulmatr_ops, s,
                          TYPE_VIRT_MULMATR, 0x500);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    s->id_reg = CHIP_ID; 
    s->control_reg = DEFAULT_CTRL_REG;
    s->size_reg = DEFAULT_SIZE_REG;
}

static void virt_mulmatr_class_init(ObjectClass *klass, void *data)
{
    qemu_log_mask(CPU_LOG_MMU, "Inside function (virt_mulmatr.c) virt_mulmatr_class_init ######################### QEMU #########################\n");

    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = virt_mulmatr_realize;
}

static const TypeInfo virt_mulmatr_info = {
    .name          = TYPE_VIRT_MULMATR,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(VirtMulMatrState),
    .class_init    = virt_mulmatr_class_init,
};

static void virt_mulmatr_register_types(void)
{
    qemu_log_mask(CPU_LOG_MMU, "Inside function (virt_mulmatr.c) virt_mulmatr_register_types ######################### QEMU #########################\n");
    
    type_register_static(&virt_mulmatr_info);
}

type_init(virt_mulmatr_register_types)