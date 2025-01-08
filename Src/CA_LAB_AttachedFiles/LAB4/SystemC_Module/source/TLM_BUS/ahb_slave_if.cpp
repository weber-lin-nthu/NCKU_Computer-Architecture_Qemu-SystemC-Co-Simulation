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

#include <ahb_slave_if.h>

ahb_slave_if::ahb_slave_if (uint32_t mapping_size) : ahb_slave_socket ("ahb_slave_socket")
{
    if (is_power_of_two (mapping_size) ) {
        ahb_slave_socket.register_b_transport (this, &ahb_slave_if::b_transport);
        this->address_mask = mapping_size - 1;
    }

    else {;}
}

void ahb_slave_if::b_transport (tlm_generic_payload& payload, sc_time& delay)
{
    tlm_command     cmd = payload.get_command();
    uint32_t        addr = (uint32_t) payload.get_address();
    uint32_t*       data_ptr = (uint32_t*) payload.get_data_ptr();
    unsigned int    length = payload.get_data_length();

    /*
    if(addr & get_address_mask() >= 0)
    {
        //printf("%u %u\n", get_base_address(), addr);
        payload.set_response_status(TLM_ADDRESS_ERROR_RESPONSE);
    }
    else*/

    if (length > 4) {
        payload.set_response_status (TLM_BURST_ERROR_RESPONSE);
    }

    else {
        bool success;
        uint32_t data;

        if (cmd == TLM_READ_COMMAND) {
            success = local_access (false, addr, data, length);

            if (success) {
                *data_ptr = data;
                payload.set_response_status (TLM_OK_RESPONSE);
            }

            else {
                payload.set_response_status (TLM_GENERIC_ERROR_RESPONSE);
            }

            //memcpy(ptr, &mem[adr], len);
        }

        else if (cmd == TLM_WRITE_COMMAND) {
            data = *data_ptr;
            success = local_access (true, addr, data, length);

            if (success) {
                payload.set_response_status (TLM_OK_RESPONSE);
            }

            else {
                payload.set_response_status (TLM_GENERIC_ERROR_RESPONSE);
            }

            //memcpy(&mem[adr], ptr, len);
        }

        else {
            payload.set_response_status (TLM_COMMAND_ERROR_RESPONSE);
        }
    }
}
uint32_t ahb_slave_if::get_address_mask()
{
    return address_mask;
}

inline bool ahb_slave_if::is_power_of_two (uint32_t x)
{
    return ( (x != 0) && ( (x & (~x + 1) ) == x) );
}

