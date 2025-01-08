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

#include <signal.h>
#include <systemc.h>
#include "top.h"

using namespace std;
unique_ptr<top> hw_top;

static void intHandler(int intid) {
	hw_top.reset( nullptr );//destruct all object
	exit(0);
}

int sc_main(int argc, char** argv) {
	signal(SIGINT, intHandler);

	hw_top.reset( new top("hw_top") );
	printf("Start Simulating.......\n");
	sc_start();
	return 0;
}
