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

#ifndef _AHB_H_
#define _AHB_H_

#include <sys/types.h>

#include <systemc.h>
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
using namespace tlm;
using namespace tlm_utils;

#include <decoder.h>
#include <systemc.h>

class ahb: public sc_module
{
    public:
        unsigned int clk_period_ns;

        multi_passthrough_target_socket<ahb> ahb_from_master_socket;
        //simple_target_socket<ahb> from_master_socket;
        multi_passthrough_initiator_socket<ahb> ahb_to_slave_socket;

        SC_HAS_PROCESS (ahb);
        ahb (sc_module_name nm);

        virtual void b_transport (int id, tlm_generic_payload& trans, sc_time& delay);
        bool add_mapping (unsigned int slave_id, uint32_t base_address, uint32_t mapping_size);

    protected:
        decoder my_decoder;

};
#endif
