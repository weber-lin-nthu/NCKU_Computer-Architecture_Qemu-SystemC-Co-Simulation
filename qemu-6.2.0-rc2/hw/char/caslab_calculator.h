#ifndef CASLAB_CALCULATOR_H
#define CASLAB_CALCULATOR_H
#include "hw/sysbus.h"
#include "chardev/char-fe.h"
#include "hw/qdev-core.h"
#include "qapi/error.h"

#define TYPE_CASLAB_CALC "caslab_calc"
#define CASLAB_CALC(obj) OBJECT_CHECK(CASLabCalcState, \
								(obj), \
								TYPE_CASLAB_CALC);

#define CASLAB_CALC_RegCount	6

typedef struct {
	SysBusDevice parent_obj;
	MemoryRegion iomem;
	
	uint32_t Reg[CASLAB_CALC_RegCount];
	CharBackend chr;
	qemu_irq irq;
	const unsigned char *id;
} CASLabCalcState;

static inline DeviceState* caslab_calc_create(hwaddr addr,
											qemu_irq irq,
											Chardev *chr)
{
	DeviceState *dev;
	SysBusDevice *s;
	dev = qdev_new(TYPE_CASLAB_CALC);
	s = SYS_BUS_DEVICE(dev);
	qdev_prop_set_chr(dev, "chardev", chr);
	qdev_realize_and_unref(dev, NULL, &error_fatal);
	sysbus_mmio_map(s, 0, addr);
	sysbus_connect_irq(s, 0, irq);
	return dev;
}

#endif
