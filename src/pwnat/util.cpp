/*
 * Copyright (C) 2009, 2013 by Daniel Meekins, Tim Diels
 *
 * This file is part of pwnat.
 *
 * pwnat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * pwnat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with pwnat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util.h"

#include <udt/udt.h>
#include <iomanip>
#include <sstream>

#include <pwnat/namespaces.h>

string get_hex_dump(const unsigned char *data, int len)
{
    int max_lines = (len / 16) + (len % 16 == 0 ? 0 : 1);
    stringstream out;
    
    for(int line = 0; line < max_lines; line++)
    {
        /* print hex */
        out << hex << setfill('0');

        out << setw(8) << line * 16 << "  ";

        for (int k=0; k<2; ++k) {
            for(int i = line * 16 + 8*k; i < line * 16 + 8*(k+1); i++)
            {
                if(i < len) {
                    out << setw(2) << static_cast<u_int16_t>(data[i]) << " "; 
                }
                else {
                    out << "   ";
                }
            }
            out << " ";
        }
        
        /* print ascii */
        out << dec << setfill(' ');
        for (int k=0; k<2; ++k) {
            for(int i = line * 16 + 8*k; i < line * 16 +  8*(k+1); i++)
            {
                if(i < len)
                {
                    if(32 <= data[i] && data[i] <= 126) {
                        out << data[i];
                    }
                    else {
                        out << ".";
                    }
                }
                else
                    out << " ";
            }
            out << " ";
        }

        out << endl;
    }

    return out.str();
}

string format_udt_error(string prefix) {
    stringstream str;
    str << prefix << ": " << UDT::getlasterror().getErrorMessage();
    return str.str();
}
