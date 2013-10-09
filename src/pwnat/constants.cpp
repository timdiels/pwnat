/*
 * Copyright (C) 2013 by Tim Diels
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

#include "constants.h"

using namespace std;

// Note: multiple UDT sockets can reuse the same port, eventually we will have a single port configured, where udp_port_c = udp_port_s, because there's no way of communicating the proxy client port number (unless including it in e.g. the IP id)
const int udp_port_c = 22201;
const int udp_port_s = 22203;

const string g_icmp_echo_destination = "127.0.0.1"; // TODO replace by 3.3.3.3

const icmphdr g_icmp_echo = {
    ICMP_ECHO, // type
    0, // code
    htons(0xf7ff), // checksum
    0, // un.echo.id
    0 // un.echo.sequence
};
