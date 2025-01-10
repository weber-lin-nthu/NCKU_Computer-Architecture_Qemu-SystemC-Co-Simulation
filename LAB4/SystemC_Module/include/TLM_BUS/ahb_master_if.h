/*
 *  Copyright (c) 2012.  	Computer Architecture and System Laboratory, CASLab
 *							Dept. of Electrical Engineering & Inst. of Computer and Communication Engineering
 *							National Cheng Kung University, Tainan, Taiwan
 *  All Right Reserved
 *
 *	Written by Chien-Te Liu (ufoderek@yahoo.com.tw)
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

#ifndef _AHB_MASTER_IF_H_
#define _AHB_MASTER_IF_H_

#include <stdint.h>

#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
using namespace tlm;
using namespace tlm_utils;

#include <tlm_payload_pool.h>
#include <systemc.h>

// Initiator module generating generic payload transactions

class ahb_master_if
{
    public:
        // TLM-2 socket, defaults to 32-bits wide, base protocol
        simple_initiator_socket<ahb_master_if> ahb_master_socket;
        ahb_master_if();
        ~ahb_master_if() {}

    protected:
        bool bus_b_access (bool write, sc_dt::uint64 addr, unsigned char* data, unsigned int length, sc_time delay = SC_ZERO_TIME);

    private:
        //tlm_payload_pool payload_pool;
};

#endif
