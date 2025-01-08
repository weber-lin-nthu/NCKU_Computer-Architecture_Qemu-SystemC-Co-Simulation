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

#include "calculator.h"

using namespace std;

#ifdef CASLAB_CALC_DEBUG
#define DB_PRINT(...)					\
	do									\
	{									\
		fprintf(stderr, "[CALC]: ");\
		fprintf(stderr, ##__VA_ARGS__);	\
	} while(0);
#else
#define DB_PRINT(...)
#endif

const unsigned char calculator::caslab_calc_id_arm[] = { 0x11, 0x10, 0x88, 0x08, 0x0d, 0xf0, 0x05, 0xb1 };

calculator::calculator(sc_module_name name,  uint32_t mapping_size) : ahb_slave_if (mapping_size), sc_module (name)
{
	SC_THREAD(run);
	sensitive << clk.pos();
	reset_signal_is( rst_in, true );
}

calculator::~calculator(void)
{
	
}

void calculator::caslab_calc_op(void)
{
	uint32_t i;
	uint32_t op;
	uint32_t src1, src2;
	uint32_t result = 0;

	op		= Reg[CALC_REG_OPERATOR];
	src1	= Reg[CALC_REG_OPERAND1];
	src2	= Reg[CALC_REG_OPERAND2];

	switch(op) {
	case CALC_OP_ADD:
		result = src1 + src2;
		wait();
		break;
	case CALC_OP_SUB:
		result = src1 - src2;
		wait();
		break;
	case CALC_OP_MUL:
		result = src1 * src2;
		for(i = 0; i < 4; i++)
			wait();
		break;
	case CALC_OP_DIV:
		if(src2 == 0)
			fprintf(stderr, "Calculator: Invalid operand_2 !!\n");
		else {
			result = src1 / src2;
			for(i = 0; i < 13; i++)
				wait();
		}
		break;
	default:
		break;
	}
	Reg[CALC_REG_RESULT] = result;
}

void calculator::run()
{
	//Reset registers
	for(unsigned i = 0; i < CASLAB_CALC_RegCount; i++)
		Reg[i] = 0x00;
	//Clear IRQ
	irq_out = false;
	state = 0;
	
	while(1) {
		wait();
		if(Reg[CALC_REG_STATUS] == CALC_STATUS_IDLE) {
			switch(Reg[CALC_REG_CONTROL]) {
			case CALC_CTRL_GO:
				irq_out = false;
				state = 0;
				Reg[CALC_REG_STATUS] = CALC_STATUS_BUSY;
				break;
			case CALC_CTRL_CLRI:
				irq_out = false;
				DB_PRINT("Reset IRQ.\n");
				break;
			default:
				break;
			}
		} else {//Run the state machine
			if(Reg[CALC_REG_CONTROL] == CALC_CTRL_ABORT) {
				state = 0;
				Reg[CALC_REG_STATUS] = CALC_STATUS_IDLE;
			} else {
				switch(state) {
				case 0:
					caslab_calc_op();
					state = state + 1;
					break;
				case 1:
					irq_out = true;
					Reg[CALC_REG_STATUS] = CALC_STATUS_IDLE;
					state = 0;
					DB_PRINT("Done, raising IRQ.\n");
					break;
				default:
					state = 0;
					break;
				}
			}
		}
		Reg[CALC_REG_CONTROL] = CALC_CTRL_IDLE;
	}
}

bool calculator::read(uint32_t* data, uint32_t addr, int length)
{
	bool success = true;
	DB_PRINT("Reading data from address 0x%08x\n", addr);
	if(addr < (CASLAB_CALC_RegCount << 2))
		*data = Reg[addr >> 2];
	else {
		switch(addr >> 2) {
		case 0x3F8 ... 0x3FF:
			*data = caslab_calc_id_arm[(addr >> 2) - 0x3F8];
			break;
		default:
			*data = 0x00;
			success = false;
		}
	}
	return success;
}

bool calculator::write(uint32_t data, uint32_t addr, int length)
{
	bool success = true;
	DB_PRINT("Writting data [0x%08x] to address 0x%08x\n", data, addr);
	if(addr < (CASLAB_CALC_RegCount << 2)) {
		switch(addr >> 2) {
		case CALC_REG_OPERATOR:
		case CALC_REG_OPERAND1:
		case CALC_REG_OPERAND2:
		case CALC_REG_CONTROL:
			Reg[addr >> 2] = data;
			break;
		default://Read only.
			success = false;
			break;
		}
	} else
		success = false;
	
	return success;
}

bool calculator::local_access (bool write, uint32_t addr, uint32_t& data, unsigned int length)
{
	bool success = true;
	uint32_t local_address = addr & get_address_mask();

	if(write)
		success = this->write(data, local_address, length);
	else
		success = this->read(&data, local_address, length);

	return success;
}
