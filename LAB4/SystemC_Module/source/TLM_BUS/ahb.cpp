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

#include <ahb.h>

ahb::ahb (sc_module_name name) : sc_module (name), ahb_from_master_socket ("ahb_from_master_socket"), ahb_to_slave_socket ("ahb_to_slave_socket")
{
    ahb_from_master_socket.register_b_transport (this, &ahb::b_transport);
}

bool ahb::add_mapping (uint32_t slave_id, uint32_t base_address, uint32_t mapping_size)
{
    return my_decoder.add_mapping (slave_id, base_address, mapping_size);
}

void ahb::b_transport (int id, tlm_generic_payload& payload, sc_time& delay)
{
    unsigned int slave_id;
    bool success = false;
    uint32_t addr = (uint32_t) payload.get_address();
    success = my_decoder.decode (slave_id, addr);

    if (!success) {
        payload.set_response_status (TLM_ADDRESS_ERROR_RESPONSE);
    }

    else {
        ahb_to_slave_socket[slave_id]->b_transport (payload, delay);
    }

}
