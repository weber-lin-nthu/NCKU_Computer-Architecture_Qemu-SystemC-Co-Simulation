/*
 *  Copyright (c) 2018.  	Computer Architecture and System Laboratory, CASLab
 *							Dept. of Electrical Engineering & Inst. of Computer and Communication Engineering
 *							National Cheng Kung University, Tainan, Taiwan
 *  All Right Reserved
 *
 *	Written by  Dun-Jie Chen (cdj@caslab.ee.ncku.edu.tw)
 *				Kuan-Chung Chen (edi@casmail.ee.ncku.edu.tw)
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

#ifndef _SYSBRG_TOP_H_
#define _SYSBRG_TOP_H_

#include <stdint.h>
#include <systemc.h>
#include <ahb.h>
#include <qsbridge.h>
#include <calculator.h>

class top : public sc_module
{
	public:
		SC_HAS_PROCESS(top);
		top(sc_module_name name);
		~top();

		sc_signal<bool> irq;
		sc_signal<bool> rst;
		unique_ptr<ahb> my_bus;
		unique_ptr<sc_clock> clk_4;

		unique_ptr<qsbridge> bridge;
		unique_ptr<calculator> cal;
};
#endif
