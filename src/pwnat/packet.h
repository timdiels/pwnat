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

/*
 * Declarations of packets
 */

#pragma once

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

struct icmp6_ttl_exceeded {
    icmp6_hdr icmp;
    ip6_hdr ip_header;
    icmp6_hdr original_icmp;
};
// TODO update readme

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
