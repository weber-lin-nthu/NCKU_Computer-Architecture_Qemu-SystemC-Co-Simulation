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

#ifndef _AHB_SLAVE_IF_H_
#define _AHB_SLAVE_IF_H_

#include <stdint.h>
using namespace std;

#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
using namespace tlm;

#include <systemc.h>

class ahb_slave_if
{
    public:
        tlm_utils::simple_target_socket<ahb_slave_if> ahb_slave_socket;

        ahb_slave_if (uint32_t mapping_size);
        ~ahb_slave_if() {}

        virtual void b_transport (tlm_generic_payload& trans, sc_time& delay);
        virtual bool local_access (bool write, uint32_t addr, uint32_t& data, unsigned int length) = 0;

        uint32_t get_address_mask();

    protected:

    private:
        uint32_t address_mask;
        inline bool is_power_of_two (uint32_t addr);
};
#endif
