#ifndef _H_QSBRIDGE_
#define _H_QSBRIDGE_
#include <semaphore.h>
#include <systemc.h>
#include <ahb_master_if.h>
#include <stdint.h>

#define	CAS_SYSBRG_SMID			0x8000
#define CAS_SYSBRG_IRQSEM		"/CASSYS_IRQ"
#define CAS_SYSBRG_DATASEM		"/CASSYS_DATA"
#define CAS_SYSBRG_ACKSEM		"/CASSYS_ACK"

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



class qsbridge: public sc_module, public ahb_master_if {
	public:
		sc_in_clk clk;
		sc_in<bool> irq_in;
		sc_out<bool> rst_out;

		SC_HAS_PROCESS(qsbridge);
		qsbridge(sc_module_name name);
		~qsbridge();

		void run();
	
	protected:
		bool bus_read(uint32_t* data, uint32_t addr, uint32_t length);
		bool bus_read64(uint64_t* data, uint64_t addr, uint32_t length);
		bool bus_write(uint32_t data, uint32_t addr, uint32_t length);
		bool bus_write64(uint64_t data, uint64_t addr, uint32_t length);
	
	private:
		sem_t* SEM_IRQ;
		sem_t* SEM_BUS;
		sem_t* SEM_BUS_Ack;
		//sem_t SEM_BUS_Lock;
		int shm_id;
		volatile CAS_SYSBUS_DATA* sh_mem;
		bool connected;
};


#endif
