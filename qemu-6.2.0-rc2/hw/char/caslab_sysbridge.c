/*
 *  Copyright (c) 2018.  	Computer Architecture and System Laboratory, CASLab
 *							Dept. of Electrical Engineering & Inst. of Computer and Communication Engineering
 *							National Cheng Kung University, Tainan, Taiwan
 *  All Right Reserved
 *
 *	Written by  Dun-Jie Chen (cdj@caslab.ee.ncku.edu.tw)
 *   
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "chardev/char-fe.h"
#include "qemu/log.h"

#include "migration/vmstate.h"
#include "hw/qdev-properties-system.h"
#include "hw/qdev-core.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "chardev/char-fe.h"
#include "hw/qdev-core.h"
#include "qapi/error.h"

#include <fcntl.h>      /* For O_* constants */
#include <sys/stat.h>   /* For mode constants */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>

#define CASLAB_SYSBRG_DEBUG

#ifdef CASLAB_SYSBRG_DEBUG
#define DB_PRINT(...)					\
	do									\
	{									\
		fprintf(stderr, "[DEBUG]: %s: ", __func__);\
		fprintf(stderr, ##__VA_ARGS__);	\
	} while(0);
#else
#define DB_PRINT(...)
#endif

#define DeviceMemRange		0x1000
#define DeviceMemMask		0x0FFF
#define TYPE_CASLAB_SYSBRG	"caslab_sysbridge"
#define CASLAB_SYSBRG(obj)	OBJECT_CHECK(CASLabBridgeState, \
										(obj), \
										TYPE_CASLAB_SYSBRG)

#define	CAS_SYSBRG_SMID		0x8000
#define CAS_SYSBRG_IRQSEM	"/CASSYS_IRQ"
#define CAS_SYSBRG_DATASEM	"/CASSYS_DATA"
#define CAS_SYSBRG_ACKSEM	"/CASSYS_ACK"

#define CAS_SYSBUS_CMD_Read		0x00
#define CAS_SYSBUS_CMD_Write	0x01
#define CAS_SYSBUS_CMD_Reset	0x02
#define CAS_SYSBUS_CMD_Exit		0x04


typedef struct {
	uint32_t Address;
	uint32_t Data;
	uint32_t ACK;//For bus access ack/nak
	uint32_t CMD;
	uint32_t ISR;
} CAS_SYSBUS_DATA;

typedef struct {
	SysBusDevice parent_obj;
	MemoryRegion iomem;
	CharBackend chr;
	qemu_irq irq;
	const unsigned char *id;
	
	sem_t* SEM_IRQ;
	sem_t* SEM_BUS;
	sem_t* SEM_BUS_Ack;
	sem_t SEM_BUS_Key;//Unnamed semaphore
	bool _connected;
	int shm_id;
	volatile CAS_SYSBUS_DATA* sh_mem;
} CASLabBridgeState;


volatile bool PrgExit = false;
static pthread_t IRQ_Thread;
static CASLabBridgeState* _dev;
void* Thread_UpdateIRQ(void*);

static const unsigned char caslab_sysbridge_id_arm[8] = 
	{0x11, 0x10, 0x88, 0x08, 0x0d, 0xf0, 0x05, 0xb1};

static void caslab_sysbridge_deinit(CASLabBridgeState* s) {
	struct shmid_ds buf;
	struct timespec wait_time = { 0 };
	DB_PRINT("SYSBUS DEINIT\n");
	clock_gettime(CLOCK_REALTIME, &wait_time);
	wait_time.tv_sec += 3;//wait 3s before force exit.
	

	if(s->_connected) {
		s->_connected = false;
		DB_PRINT("Disonnceting from HW.\n");
		DB_PRINT("Invoking HW Bus Signal.\n")
		s->sh_mem->CMD = CAS_SYSBUS_CMD_Exit;
		while(sem_trywait(s->SEM_BUS_Ack) == 0);
		sem_post(s->SEM_BUS);
		sem_timedwait(s->SEM_BUS_Ack, &wait_time);
		//reset sems.
		while(sem_trywait(s->SEM_BUS) == 0);
		while(sem_trywait(s->SEM_BUS_Ack) == 0);
		while(sem_trywait(s->SEM_IRQ) == 0);
		sem_close(s->SEM_BUS);
		sem_close(s->SEM_BUS_Ack);
		sem_close(s->SEM_IRQ);
	}


	shmdt((void*)s->sh_mem);
	sem_destroy(&s->SEM_BUS_Key);
	if(shmctl(s->shm_id, IPC_STAT, &buf) == 0)
		shmctl(s->shm_id, IPC_RMID, &buf);//Delete shared memory.
}

static void ON_Module_Exit(void) {
	PrgExit = true;
	if(_dev != NULL) {
		sem_post(_dev->SEM_IRQ);
		//pthread_join(IRQ_Thread, NULL);
		pthread_cancel(IRQ_Thread);
		caslab_sysbridge_deinit(_dev);
	}
}

void* Thread_UpdateIRQ(void *mem) {
	CASLabBridgeState *s;
	s = (CASLabBridgeState*) mem;


	while(!PrgExit) {
		if(s->_connected) {
			uint32_t ISR;
			DB_PRINT("Waiting Device IRQ Signal.\n");
			sem_wait(s->SEM_IRQ);
			if(PrgExit) break;
			if(s->sh_mem->CMD == CAS_SYSBUS_CMD_Exit) {
				sem_wait(&s->SEM_BUS_Key);
				s->sh_mem->CMD = CAS_SYSBUS_CMD_Read;
				sem_post(&s->SEM_BUS_Key);
				s->_connected = false;
				DB_PRINT("Device Disconnected.\n");
				while(sem_trywait(s->SEM_BUS) == 0);
				while(sem_trywait(s->SEM_BUS_Ack) == 0);
				while(sem_trywait(s->SEM_IRQ) == 0);
				continue;
			}

			ISR = s->sh_mem->ISR;
			DB_PRINT("Received IRQ Signal from HW.(0x%02x)\n", ISR);
			qemu_mutex_lock_iothread();// It's inside of pthread other than qemu main thread, needed to be lock befor using any internal IO resource

			if(ISR & 0x01)
				qemu_irq_raise(s->irq);
			else
				qemu_irq_lower(s->irq);

			qemu_mutex_unlock_iothread();// Done, now unlock it.
		} else {
			DB_PRINT("Waitting Device up...\n");
			sem_post(s->SEM_BUS);
			sem_wait(s->SEM_BUS_Ack);
			s->_connected = true;
			DB_PRINT("Device connected.\n");
			if(PrgExit) break;
		}
	}
	DB_PRINT("IRQ Listener exited.\n");
	return NULL;
}

static const VMStateDescription vmstate_caslab_sysbridge = {
	.name				= TYPE_CASLAB_SYSBRG,
	.version_id			= 1,
	.minimum_version_id	= 1,
	.fields				= (VMStateField[]) {//nothing needed to be saved here.
		VMSTATE_END_OF_LIST(),
	}
};

static uint64_t caslab_sysbridge_read(void *opaque, hwaddr offset, unsigned size) {
	CASLabBridgeState *s = opaque;
	uint64_t ret = 0;
	hwaddr temp;
	switch(offset & DeviceMemMask) {
	case 0xFE0 ... 0xFFC:
		temp = (offset & DeviceMemMask) - 0xFE0;
		ret = caslab_sysbridge_id_arm[temp >> 2];
		break;
	default:
		if(s->_connected && offset < DeviceMemRange) {
			sem_wait(&s->SEM_BUS_Key);//Acquire the key to access BUS

			s->sh_mem->CMD = CAS_SYSBUS_CMD_Read;
			s->sh_mem->Address = offset;
			sem_post(s->SEM_BUS);
			sem_wait(s->SEM_BUS_Ack);
			ret = s->sh_mem->Data;

			sem_post(&s->SEM_BUS_Key);//Release the key
		}
		break;
	}
	return ret;
}

static void caslab_sysbridge_write(void *opaque, hwaddr offset, uint64_t value, unsigned size) {
	CASLabBridgeState *s = opaque;
	if(offset >= DeviceMemRange) {
		fprintf(stderr, "Device ERROR: Overflow!!!\n");
		return;
	}
	if(!s->_connected)
		return;

	sem_wait(&s->SEM_BUS_Key);//Acquire the key to access BUS

	s->sh_mem->CMD = CAS_SYSBUS_CMD_Write;
	s->sh_mem->Address = offset;
	s->sh_mem->Data = (uint32_t)value;
	sem_post(s->SEM_BUS);
	sem_wait(s->SEM_BUS_Ack);

	sem_post(&s->SEM_BUS_Key);//Release the key
}

static void caslab_sysbridge_reset(DeviceState *dev) {
	CASLabBridgeState *s = CASLAB_SYSBRG(dev);
	
	if(!s->_connected)
		return;

	sem_wait(&s->SEM_BUS_Key);//Acquire the key to access BUS
	s->sh_mem->CMD = CAS_SYSBUS_CMD_Reset;
	sem_post(s->SEM_BUS);
	sem_wait(s->SEM_BUS_Ack);
	sem_post(&s->SEM_BUS_Key);//Release the key
}

static void caslab_sysbridge_realize(DeviceState *dev, Error **errp) {
	//Nothing to do here currently.
}

static const MemoryRegionOps caslab_sysbridge_ops = {
	.read			= caslab_sysbridge_read,
	.write			= caslab_sysbridge_write,
	.endianness		= DEVICE_NATIVE_ENDIAN,
	.valid.min_access_size	= 4,
	.valid.max_access_size	= 4,
};

static void caslab_sysbridge_init(Object *obj) {
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
	CASLabBridgeState *s = CASLAB_SYSBRG(obj);
	int init_state = 5;
	struct shmid_ds buf;
	s->_connected = false;
	s->shm_id = -1;
	
	memory_region_init_io(&s->iomem, OBJECT(s), &caslab_sysbridge_ops, s,
							TYPE_CASLAB_SYSBRG, DeviceMemRange);

	sysbus_init_mmio(sbd, &s->iomem);
	sysbus_init_irq(sbd, &s->irq);
	do {
		if(0 != sem_init(&s->SEM_BUS_Key, 0, 1)) //Assign only 1 key to act like mutex
			break;
		init_state--;

		s->SEM_BUS = sem_open(CAS_SYSBRG_DATASEM, O_CREAT, 0666, 0);
		s->SEM_BUS_Ack = sem_open(CAS_SYSBRG_ACKSEM, O_CREAT, 0666, 0);
		s->SEM_IRQ = sem_open(CAS_SYSBRG_IRQSEM, O_CREAT, 0666, 0);
		if(SEM_FAILED == s->SEM_BUS || SEM_FAILED == s->SEM_BUS_Ack || SEM_FAILED == s->SEM_IRQ)
			break;
		init_state--;
		
		//Reset sems
		while(sem_trywait(s->SEM_BUS) == 0);
		while(sem_trywait(s->SEM_BUS_Ack) == 0);
		while(sem_trywait(s->SEM_IRQ) == 0);

		s->shm_id = shmget(CAS_SYSBRG_SMID, sizeof(CAS_SYSBUS_DATA), 0666 | IPC_CREAT);
		if(s->shm_id == -1)
			break;
		init_state--;

		s->sh_mem = shmat(s->shm_id, NULL, 0);
		if(s->sh_mem == (void*)-1)
			break;
		init_state--;

		memset((void*)s->sh_mem, 0, sizeof(CAS_SYSBUS_DATA));
		if(0 != pthread_create(&IRQ_Thread, NULL, Thread_UpdateIRQ, (void*)s))
			break;
		init_state--;
	} while(0);

	switch(init_state) {
	case 0:
		DB_PRINT("SYSBUS INIT\n");
		break;
	case 1://pthread_create failed
		fprintf(stderr, "ERROR: pthread failed\n");
		shmdt((void*) s->sh_mem);
		s->sh_mem = NULL;
	case 2://shmat failed
		fprintf(stderr, "ERROR: shmat failed\n");
		if(shmctl(s->shm_id, IPC_STAT, &buf) == 0)
			shmctl(s->shm_id, IPC_RMID, &buf);
	case 3://shmget failed
		fprintf(stderr, "ERROR: shmget failed\n");
		while(sem_trywait(s->SEM_BUS) == 0);
		while(sem_trywait(s->SEM_BUS_Ack) == 0);
		while(sem_trywait(s->SEM_IRQ) == 0);
		sem_close(s->SEM_BUS);
		sem_close(s->SEM_BUS_Ack);
		sem_close(s->SEM_IRQ);
	case 4://shared semaphore failed
		fprintf(stderr, "ERROR: semaphore failed\n");
		sem_destroy(&s->SEM_BUS_Key);
	case 5://anonymous semaphore failed
		fprintf(stderr, "ERROR: semaphore failed\n");
	default:
		exit(-1);
		break;
	}

	_dev = s;
	if(0 != atexit(ON_Module_Exit))
		fprintf(stderr, "failed to register on exit function, may cause memory violation when exit.\n");
	
}

static Property caslab_sysbridge_properties[] = {
	DEFINE_PROP_CHR("chardev", CASLabBridgeState, chr),
	DEFINE_PROP_END_OF_LIST(),
};

static void caslab_sysbridge_class_init(ObjectClass *oc, void *data) {
	DeviceClass *dc = DEVICE_CLASS(oc);

	dc->realize		= caslab_sysbridge_realize;
	dc->vmsd		= &vmstate_caslab_sysbridge;
	dc->reset		= caslab_sysbridge_reset;
	dc->props_		= caslab_sysbridge_properties;
}

static const TypeInfo caslab_sysbridge_info = {
	.name			= TYPE_CASLAB_SYSBRG,
	.parent			= TYPE_SYS_BUS_DEVICE,
	.instance_size	= sizeof(CASLabBridgeState),
	.instance_init	= caslab_sysbridge_init,
	.class_init		= caslab_sysbridge_class_init
};

static void caslab_sysbridge_register_types(void) {
	type_register_static(&caslab_sysbridge_info);
}

type_init(caslab_sysbridge_register_types)
