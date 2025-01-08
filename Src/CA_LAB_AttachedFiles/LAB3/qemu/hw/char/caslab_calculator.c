#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "qemu/log.h"

#include "caslab_calculator.h"

//#define CASLAB_CALC_DEBUG

#ifdef CASLAB_CALC_DEBUG
#define DB_PRINT(...)						\
	do										\
	{										\
		fprintf(stderr, "[DEBUG]: %s: ", __func__);\
		fprintf(stderr, ##__VA_ARGS__);		\
	} while(0);
#else
#define DB_PRINT(...)
#endif

#define	CALC_REG_CONTROL	0x00
#define	CALC_REG_OPERATOR	0x01
#define	CALC_REG_OPERAND1	0x02
#define	CALC_REG_OPERAND2	0x03
#define	CALC_REG_RESULT		0x04
#define	CALC_REG_STATUS		0x05

#define CALC_CTRL_GO		0x01
#define CALC_CTRL_ABORT		0x02
#define CALC_CTRL_CLRI		0x04

#define CALC_STATUS_IDLE	0x00
#define CALC_STATUS_BUSY	0x01

#define CALC_OP_ADD			0x01
#define CALC_OP_SUB			0x02
#define CALC_OP_MUL			0x03
#define CALC_OP_DIV			0x04

static void caslab_calc_reset(DeviceState *dev); //forward decleration

static void caslab_calc_op(CASLabCalcState *s)
{
	uint32_t op;
	uint32_t src1, src2;
	uint32_t result = 0;
	if(s == NULL)
		return;
	s->Reg[CALC_REG_STATUS] = CALC_STATUS_BUSY;

	op		= s->Reg[CALC_REG_OPERATOR];
	src1	= s->Reg[CALC_REG_OPERAND1];
	src2	= s->Reg[CALC_REG_OPERAND2];

	DB_PRINT("Doing OP %u with value %u, %u\n", op, src1, src2);
	switch(op) {
	case CALC_OP_ADD:
		result = src1 + src2;
		break;
	case CALC_OP_SUB:
		result = src1 - src2;
		break;
	case CALC_OP_MUL:
		result = src1 * src2;
		break;
	case CALC_OP_DIV:
		if(src2 == 0)
			fprintf(stderr, "Calculator: Invalid operand_2 !!\n");
		else
			result = src1 / src2;
		break;
	default:
		result = 0;
		break;
	}
	s->Reg[CALC_REG_STATUS] = CALC_STATUS_IDLE;
	s->Reg[CALC_REG_RESULT] = result;
	qemu_irq_raise(s->irq);
}

static const unsigned char caslab_calc_id_arm[8] = 
	{0x11, 0x10, 0x88, 0x08, 0x0d, 0xf0, 0x05, 0xb1};

static uint64_t caslab_calc_read(void *opaque, hwaddr offset, unsigned size)
{
	CASLabCalcState *s = opaque;
	uint64_t ret = 0;
	offset >>= 2;
	switch(offset) {
		case 0x3F8:
			ret = caslab_calc_id_arm[0];
			break;
		case 0x3F9:
			ret = caslab_calc_id_arm[1];
			break;
		case 0x3FA:
			ret = caslab_calc_id_arm[2];
			break;
		case 0x3FB:
			ret = caslab_calc_id_arm[3];
			break;
		case 0x3FC:
			ret = caslab_calc_id_arm[4];
			break;
		case 0x3FD:
			ret = caslab_calc_id_arm[5];
			break;
		case 0x3FE:
			ret = caslab_calc_id_arm[6];
			break;
		case 0x3FF:
			ret = caslab_calc_id_arm[7];
			break;

		case CALC_REG_OPERAND1:
		case CALC_REG_OPERAND2:
		case CALC_REG_RESULT:
		case CALC_REG_STATUS:
			ret = s->Reg[offset];
			break;

		default:
			break;
	};
	DB_PRINT("Read value %lu @ 0x%02lx\n", ret, offset<<2);
	return ret;
}

static void caslab_calc_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
	CASLabCalcState *s = opaque;
	DB_PRINT("Write value %lu @ 0x%02lx\n", value, offset);
	offset >>= 2;
	if(offset >= CASLAB_CALC_RegCount)
		return;
	else
	{
		switch(offset)
		{
			case CALC_REG_CONTROL:
				if(CALC_STATUS_IDLE == s->Reg[CALC_REG_STATUS]) {
					if( value == CALC_CTRL_GO ) {
						DB_PRINT("CASlab Calculator commit\n");
						caslab_calc_op(s);
					} else if ( value == CALC_CTRL_CLRI ) {
						qemu_set_irq(s->irq, 0);
					}
				} else if(CALC_STATUS_BUSY == s->Reg[CALC_REG_STATUS]) {
					if ( value == CALC_CTRL_ABORT ) {
						//Cancel operations
						s->Reg[CALC_REG_STATUS] = CALC_STATUS_IDLE;
						qemu_set_irq(s->irq, 1);
					}
				}
				break;
			case CALC_REG_OPERATOR://Writable register
			case CALC_REG_OPERAND1:
			case CALC_REG_OPERAND2:
				if(s->Reg[CALC_REG_STATUS] == CALC_STATUS_IDLE)
					s->Reg[offset] = value;
				break;
			default: //Read only register
				break;
		}
	}
}

static void caslab_calc_realize(DeviceState *dev, Error **errp)
{
	//Nothing here curreltly
}

static void caslab_calc_reset(DeviceState *dev)
{
	CASLabCalcState *s = CASLAB_CALC(dev);
	memset(s->Reg, 0, CASLAB_CALC_RegCount * sizeof(*(s->Reg)));
	qemu_set_irq(s->irq, 0);
}


static const MemoryRegionOps caslab_calc_ops = {
	.read		= caslab_calc_read,
	.write		= caslab_calc_write,
	.endianness	= DEVICE_NATIVE_ENDIAN,
	.valid.min_access_size = 4,
	.valid.max_access_size = 4
};

static void caslab_calc_init(Object *obj)
{
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
	CASLabCalcState *s = CASLAB_CALC(obj);

	memory_region_init_io(&s->iomem, OBJECT(s), &caslab_calc_ops, s,
						TYPE_CASLAB_CALC, 0x1000);
	sysbus_init_mmio(sbd, &s->iomem);
	sysbus_init_irq(sbd, &s->irq);
	DB_PRINT("INIT\n");
}

static const VMStateDescription vmstate_caslab_calc = {
	.name 				= TYPE_CASLAB_CALC,
	.version_id			= 1,
	.minimum_version_id	= 1,
	.fields	= (VMStateField[]) {
		VMSTATE_UINT32_ARRAY(Reg, CASLabCalcState,
							CASLAB_CALC_RegCount),
		VMSTATE_END_OF_LIST()
	}
};

static Property caslab_calc_properties[] = {
	DEFINE_PROP_CHR("chardev", CASLabCalcState, chr),
	DEFINE_PROP_END_OF_LIST(),
};

static void caslab_calc_class_init(ObjectClass *oc, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(oc);
	
	dc->realize = caslab_calc_realize;
	dc->vmsd 	= &vmstate_caslab_calc;
	dc->reset	= caslab_calc_reset;
	dc->props	= caslab_calc_properties;
}

static const TypeInfo caslab_calc_info = {
	.name			= TYPE_CASLAB_CALC,
	.parent			= TYPE_SYS_BUS_DEVICE,
	.instance_size	= sizeof(CASLabCalcState),
	.instance_init	= caslab_calc_init,
	.class_init		= caslab_calc_class_init,
};

static void caslab_calc_register_types(void)
{
	type_register_static(&caslab_calc_info);
}

type_init(caslab_calc_register_types)
