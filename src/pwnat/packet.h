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

#pragma once

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

/**
 * Declarations of packets
 */
struct icmp_ttl_exceeded {
    icmphdr icmp;
    ip ip_header;
    icmphdr original_icmp;
};

/**
 * ProxyClient sends this to ProxyServer to initialize a newly connected UDT flow
 */
struct udt_flow_init {
    u_int16_t size; // size of flow_init, including remote_host chars
    u_int16_t remote_port;
    // char* remote_host, not zero terminated
};
