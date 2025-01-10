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

#ifndef _TLM_PAYLOAD_POOL_
#define _TLM_PAYLOAD_POOL_

#ifndef SC_INCLUDE_DYNAMIC_PROCESSES
#define SC_INCLUDE_DYNAMIC_PROCESSES
#endif

#include <cstdio>
#include <stack>
#include <systemc.h>
#include <tlm.h>
using namespace std;
using namespace tlm;
#include <systemc.h>

class tlm_payload_pool: public tlm::tlm_mm_interface
{
    public:
        tlm_payload_pool() {
            // allocate 5 free payloads before simulation
            for (int i = 0; i < 5; i++) {
                pool.push (new tlm_generic_payload (this) );
            }
        }

        ~tlm_payload_pool() {
            while (!pool.empty() ) {
                //stack.top().reset();  // free the payload extensions, which we didn't use
                delete pool.top();
                pool.pop();
            }
        }

        tlm_generic_payload* allocate() {
            tlm_generic_payload* payload;

            if (pool.empty() ) {
                payload = new tlm_generic_payload (this);
            }

            else {
                payload = pool.top();
                pool.pop();
            }

            return payload;
        }

        void free (tlm_generic_payload* junk) {
            pool.push (junk);
        }

        virtual const char* get_debug_header() {
            return "trans_pool";
        }

    private:
        stack<tlm_generic_payload*> pool;
};
#endif

