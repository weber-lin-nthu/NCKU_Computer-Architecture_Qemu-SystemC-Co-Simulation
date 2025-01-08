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

#include <decoder.h>

decoder::decoder()
{
    uint32_t i;
    //printm (d_decoder, "allocating memory...");
    map = new char[map_size];
    //printm (d_decoder, "memory allocated");

    for (i = 0; i < map_size; i++) {
        map[i] = -1;
    }

    printf("map_size: 0x%X, check i: 0x%X\n", map_size, i);
}

decoder::~decoder()
{
    if (map != 0) {
        delete[] map;
    }
}

bool decoder::add_mapping (unsigned int slave_id, uint32_t base_address, uint32_t mapping_size)
{
    uint32_t end_address;
    end_address = ( (base_address + mapping_size) >> shift);
    base_address >>= shift;
    printf("mapping_size: %d (KiB), slave id: %u\n", mapping_size / 1024, slave_id);
    printf("base address: 0x%X, end address: 0x%X\n", base_address << shift, end_address << shift);

    for (uint32_t i = base_address; i < end_address; i++) {
        if (map[i] == -1) {
            map[i] = slave_id;
        }

        else {
            printf("mapping overlapped\n");
			exit(1);
        }
    }

    return true;
}

bool decoder::decode (unsigned int& slave_id, uint32_t address)
{
    uint32_t inter_addr = address >> shift;

    if (inter_addr < map_size) {
        int tmp_id = map[inter_addr];

        if (tmp_id >= 0) {
            slave_id = tmp_id;
            return true;
        }
    }

    return false;
}

