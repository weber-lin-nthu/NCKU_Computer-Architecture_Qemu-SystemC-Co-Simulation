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

#include <stdint.h>
#include <systemc.h>

class decoder
{
    public:
        decoder();
        ~decoder();

        bool add_mapping (unsigned int slave_id, uint32_t base_address, uint32_t mapping_size);
        bool decode (unsigned int& slave_id, uint32_t address);

    private:
        static const uint32_t shift = 10;
        static const uint32_t map_size = 0xFFFFFFFF >> shift;
        char* map;
};

