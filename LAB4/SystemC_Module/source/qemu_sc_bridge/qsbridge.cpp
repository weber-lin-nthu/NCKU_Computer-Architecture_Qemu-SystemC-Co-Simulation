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

#include "qsbridge.h"
#include <signal.h>
#include <fcntl.h>		/* For O_* constants */
#include <sys/ipc.h>
#include <sys/shm.h>

using namespace std;

#define CASLAB_SYSBRG_DEBUG // 新增此行

#ifdef CASLAB_SYSBRG_DEBUG
#define DB_PRINT(...)					\
	do									\
	{									\
		fprintf(stderr, "[QSBRIDGE]: ");\
		fprintf(stderr, ##__VA_ARGS__);	\
	} while(0);
#else
#define DB_PRINT(...)
#endif


qsbridge::qsbridge(sc_module_name name): sc_module(name)
{
	int init_state = 3;
	shm_id = -1;
	connected = false;
	do {
		SEM_IRQ = sem_open(CAS_SYSBRG_IRQSEM, O_CREAT, S_IRUSR | S_IWUSR, 0);
		SEM_BUS = sem_open(CAS_SYSBRG_DATASEM, O_CREAT, S_IRUSR | S_IWUSR, 0);
		SEM_BUS_Ack = sem_open(CAS_SYSBRG_ACKSEM, O_CREAT, S_IRUSR | S_IWUSR, 0);
		if(SEM_FAILED == SEM_IRQ || SEM_FAILED == SEM_BUS || SEM_FAILED == SEM_BUS_Ack) {
			fprintf(stderr, "ERROR: semaphore failed\n");
			break;
		}
		init_state--;
		//Clear semaphore
		while(sem_trywait(SEM_IRQ) == 0);
		while(sem_trywait(SEM_BUS_Ack) == 0);

		shm_id = shmget(CAS_SYSBRG_SMID, sizeof(CAS_SYSBUS_DATA), 0666 | IPC_CREAT);
		if(shm_id == -1){
			fprintf(stderr, "ERROR: shmget failed\n");
			break;
		}
		init_state--;

		sh_mem = (volatile CAS_SYSBUS_DATA*)shmat(shm_id, NULL, 0);
		if(sh_mem == (void*)-1) {
			fprintf(stderr, "ERROR: shmat failed\n");
			break;
		}
		memset((void*)sh_mem, 0, sizeof(CAS_SYSBUS_DATA));
		init_state--;

	} while(0);

	if(init_state != 0) {
		struct shmid_ds buf;
		switch(init_state) {
		case 1://shmat failed
			sh_mem = NULL;
			if(shm_id != -1 && shmctl(shm_id, IPC_STAT, &buf) == 0)
				shmctl(shm_id, IPC_RMID, &buf);//Delete shared memory.
		case 2://shmget failed
			sem_close(SEM_IRQ);
			while(sem_trywait(SEM_BUS) == 0);
			sem_close(SEM_BUS);
			sem_close(SEM_BUS_Ack);
		case 3://sem_open failed
			SEM_IRQ = NULL;
			SEM_BUS = NULL;
			SEM_BUS_Ack = NULL;
			exit(-1);
			break;
		default://Should not happen
			exit(0);
			break;
		}
	}

	SC_THREAD(run);
	sensitive << clk.pos();
	dont_initialize();
}

qsbridge::~qsbridge()
{
	DB_PRINT("qsbridge deinit.\n");
	if(SEM_BUS != NULL) {
		while(sem_trywait(SEM_BUS) == 0);
		sem_close(SEM_BUS);
	}
	if(SEM_BUS_Ack != NULL) {
		while(sem_trywait(SEM_BUS_Ack) == 0);
		sem_close(SEM_BUS_Ack);
	}
	if(SEM_IRQ != NULL) {
		if(sh_mem != NULL && connected) {
			DB_PRINT("Sending exit signal(0x%02x) to QEMU\n", CAS_SYSBUS_CMD_Exit);
			sh_mem->CMD = CAS_SYSBUS_CMD_Exit;
			sem_post(SEM_IRQ);
		}
		sem_close(SEM_IRQ);
	}
	if(sh_mem != NULL)
		shmdt((void*) sh_mem);
	if(shm_id != -1) {
		struct shmid_ds buf;
		if(shmctl(shm_id, IPC_STAT, &buf) == 0)
			shmctl(shm_id, IPC_RMID, &buf);//Delete shared memory.
	}
	
}

void qsbridge::run()
{
	bool success = false;
	bool _previous_irq = false;
	printf("Waiting QEMU UP....\n");
	sem_wait(SEM_BUS);
	sem_post(SEM_BUS_Ack);
	printf("Connected to QEMU.\n");
	connected = true;

	while(connected) {
		/* At first, we handle the IRQ */
		if(_previous_irq != irq_in) {
			if(irq_in)
				sh_mem->ISR |= 0x01;
			else
				sh_mem->ISR &= (uint32_t)(-1) ^ 0x01;
			sem_post(SEM_IRQ);
			_previous_irq = irq_in;
		}

		/* And then, we process the memory access */
		if(sem_trywait(SEM_BUS) == 0) {//There's bus command
			uint32_t temp;
			switch(sh_mem->CMD) {
			case CAS_SYSBUS_CMD_Read:
				DB_PRINT("BUS Read, read from 0x%08x\n", sh_mem->Address);
				success = this->bus_read(&temp, sh_mem->Address, 4);
				sh_mem->Data = temp;
				break;
			case CAS_SYSBUS_CMD_Write:
				DB_PRINT("BUS Write, Write 0x%04x to 0x%08x\n", sh_mem->Data, sh_mem->Address);
				success = this->bus_write(sh_mem->Data, sh_mem->Address, 4);
				break;
			case CAS_SYSBUS_CMD_Reset:
				DB_PRINT("BUS Reset\n");
				rst_out = true;
				wait();//Wait 1 cycle
				rst_out = false;
				success = true;
				break;
			case CAS_SYSBUS_CMD_Exit:
				DB_PRINT("Simulation end\n");
				success = true;
				connected = false;
				break;
			}
			sh_mem->ACK = (success)? 0x01:0x00;
			sem_post(SEM_BUS_Ack);
			if(!connected) break;
		}
		
		wait();/* wait for next clock trigger */
	}
	raise(SIGINT);
}

/*=================== TLM Bus Access Function ====================*/
bool qsbridge::bus_read(uint32_t* data, uint32_t addr, uint32_t length)
{
    bool success;
    success = bus_b_access(false, (sc_dt::uint64)addr, reinterpret_cast<unsigned char*>(data), length);

    if(!success) {
        DB_PRINT("bus read 0x%X failed\n", addr);
	}

	return success;
}

bool qsbridge::bus_read64(uint64_t* data, uint64_t addr, uint32_t length)
{
    bool success1;
    bool success2;
    uint32_t tmp;

    success1 = bus_b_access(false, (sc_dt::uint64)addr, reinterpret_cast<unsigned char*>(&tmp), length);
    *data = tmp;
    success2 = bus_b_access(false, (sc_dt::uint64)addr + 4, reinterpret_cast<unsigned char*>(&tmp), length);
    *data |= ((uint64_t)tmp) << 32;

    if(success1 && success2)
        return true;
    else
    {
        DB_PRINT("bus read 64 0x%lX failed\n", addr);
        return false;
    }
}

bool qsbridge::bus_write(uint32_t data, uint32_t addr, uint32_t length)
{
    bool success;
    success = bus_b_access(true, (sc_dt::uint64)addr, reinterpret_cast<unsigned char*>(&data), length);

    if(!success) {
        DB_PRINT("bus write 0x%X:0x%X failed\n", addr, data);
	}
	return success;
}

bool qsbridge::bus_write64(uint64_t data, uint64_t addr, uint32_t length)
{
    bool success1;
    bool success2;

    uint32_t tmp;

    tmp = data;
    success1 = bus_b_access(true, (sc_dt::uint64)addr, reinterpret_cast<unsigned char*>(&tmp), length);

    tmp = data >> 32;
    success2 = bus_b_access(true, (sc_dt::uint64)addr + 4, reinterpret_cast<unsigned char*>(&tmp), length);

    if(success1 && success2)
        return true;
    else
    {
        DB_PRINT("bus write 64 0x%lX:0x%lX failed\n", addr, data);
        return false;
    }
}
