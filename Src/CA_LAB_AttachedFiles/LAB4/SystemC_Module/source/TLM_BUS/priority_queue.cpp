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

#include <priority_queue.h>

void priority_queue::add_by_id_and_weight (unsigned int id, unsigned int weight)
{
    unsigned int i;

    for (i = 0; i < weight; i++) {
        queue.push_back (id);
    }
}

unsigned int priority_queue::get_id_by_order (unsigned int order)
{
    return queue[order];
}

bool priority_queue::pop_front_push_back()
{
    if (get_total_weight() > 0) {
        queue.push_back (queue.front() );
        queue.pop_front();
        return true;
    }

    return false;
}

unsigned int priority_queue::get_total_weight()
{
    return queue.size();
}


