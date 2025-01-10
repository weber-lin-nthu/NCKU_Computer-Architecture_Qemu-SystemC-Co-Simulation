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
#include <top.h>

top::~top(void) {
	printf("Top Deconstructor\n");
}

top::top(sc_module_name name) : sc_module(name) {
	uint32_t slave_id = 0;
	clk_4.reset( new sc_clock("clk_4", 250.0, SC_NS, 125.0, 0, SC_MS, true)); //4MHz Clock
	my_bus.reset( new ahb ("ahb_bus") );

	bridge.reset( new qsbridge("QSBRIDGE") );
	bridge->clk( *clk_4 );
	bridge->irq_in( irq );
	bridge->rst_out( rst );
	bridge->ahb_master_socket.bind( my_bus->ahb_from_master_socket );

	cal.reset(new calculator("cal", CAL_MEM_Region));
	cal->clk( *clk_4 );
	cal->irq_out( irq );
	cal->rst_in( rst );
	my_bus->ahb_to_slave_socket.bind( cal->ahb_slave_socket );
	my_bus->add_mapping( slave_id++, 0x0, CAL_MEM_Region );
}
