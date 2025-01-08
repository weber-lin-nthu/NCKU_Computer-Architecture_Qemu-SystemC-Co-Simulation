/*
 *  Copyright (c) 2012.  	Computer Architecture and System Laboratory, CASLab
 *							Dept. of Electrical Engineering & Inst. of Computer and Communication Engineering
 *							National Cheng Kung University, Tainan, Taiwan
 *  All Right Reserved
 * 
 * 	Written by Chien-Te Liu (ufoderek@yahoo.com.tw)
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

#include <ahb_master_if.h>

ahb_master_if::ahb_master_if() : ahb_master_socket ("ahb_master_socket")
{
}

bool ahb_master_if::bus_b_access (bool write, sc_dt::uint64 addr, unsigned char* data, unsigned int length, sc_time delay)
{
    tlm_generic_payload payload;
    payload.set_command (write ? TLM_WRITE_COMMAND : TLM_READ_COMMAND);
    payload.set_address (addr);
    payload.set_data_ptr (data);
    payload.set_data_length (length);
    payload.set_streaming_width (length);   // = data_length, means no streaming
    payload.set_byte_enable_ptr (0);
    payload.set_dmi_allowed (false);
    payload.set_response_status (TLM_INCOMPLETE_RESPONSE);

    if (delay.to_seconds() > 0.0) {
        wait (delay);
    }

    ahb_master_socket->b_transport (payload, delay);

    if (payload.is_response_error() ) {
        return false;
    }

    return true;
}

