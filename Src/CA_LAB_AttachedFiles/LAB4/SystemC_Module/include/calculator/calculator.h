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

#ifndef _H_CALCULATOR_
#define _H_CALCULATOR_
#include <systemc.h>
#include <ahb_slave_if.h>
#include <stdint.h>

#define CASLAB_CALC_RegCount	6
#define CAL_MEM_Region			0x1000

#define	CALC_REG_CONTROL	0x00
#define	CALC_REG_OPERATOR	0x01
#define	CALC_REG_OPERAND1	0x02
#define	CALC_REG_OPERAND2	0x03
#define	CALC_REG_RESULT		0x04
#define	CALC_REG_STATUS		0x05

#define CALC_CTRL_IDLE		0x00
#define CALC_CTRL_GO		0x01
#define CALC_CTRL_ABORT		0x02
#define CALC_CTRL_CLRI		0x04

#define CALC_STATUS_IDLE	0x00
#define CALC_STATUS_BUSY	0x01

#define CALC_OP_ADD			0x01
#define CALC_OP_SUB			0x02
#define CALC_OP_MUL			0x03
#define CALC_OP_DIV			0x04


class calculator:public ahb_slave_if, public sc_module
{
	public:
		sc_in_clk clk;
		sc_out<bool> irq_out;
		sc_in<bool> rst_in;

		sc_uint<32> Reg[CASLAB_CALC_RegCount];
		sc_uint<8> state;

		SC_HAS_PROCESS(calculator);
		calculator(sc_module_name name, uint32_t mapping_size);
		~calculator(void);

		void run();
		bool read (uint32_t*, uint32_t, int);
		bool write (uint32_t, uint32_t, int);
		virtual bool local_access (bool write, uint32_t addr, uint32_t& data, unsigned int length);

	private:
		void caslab_calc_op(void);
		const static unsigned char caslab_calc_id_arm[];
};
#endif
