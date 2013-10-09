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

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <pwnat/udtservice/UDTService.h>
#include <pwnat/UDTSocket.h>
#include <pwnat/Socket.h>
#include <pwnat/constants.h>
#include <pwnat/checksum.h>
#include <pwnat/packet.h>

using namespace std;

/*
 * A TCPClient that is acquiring an UDT connection to the proxy server
 */
class ConnectingTCPClient {
public:
    ConnectingTCPClient(UDTService& udt_service, boost::asio::ip::tcp::socket* tcp_socket) :
        m_tcp_socket(tcp_socket),
        m_icmp_socket(tcp_socket->get_io_service(), boost::asio::ip::icmp::endpoint(boost::asio::ip::icmp::v4(), 0))
    {
        m_icmp_socket.connect(boost::asio::ip::icmp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 0));

        // build m_icmp_ttl_exceeded
        m_icmp_ttl_exceeded.icmp.type = ICMP_TIME_EXCEEDED;
        m_icmp_ttl_exceeded.icmp.code = 0;
        m_icmp_ttl_exceeded.icmp.checksum = 0;
        memset(&m_icmp_ttl_exceeded.icmp.un, 0, 4);

        m_icmp_ttl_exceeded.ip_header.ip_hl = 5;
        m_icmp_ttl_exceeded.ip_header.ip_v = 4;
        m_icmp_ttl_exceeded.ip_header.ip_tos = 0;
        m_icmp_ttl_exceeded.ip_header.ip_len = htons(sizeof(ip) + sizeof(icmphdr));
        m_icmp_ttl_exceeded.ip_header.ip_id = htons(256);  // any random value other than 0 is probably fine
        m_icmp_ttl_exceeded.ip_header.ip_off = IP_DF;  // set don't fragment flag (is more realistic)
        m_icmp_ttl_exceeded.ip_header.ip_ttl = 1;
        m_icmp_ttl_exceeded.ip_header.ip_p = IPPROTO_ICMP;
        m_icmp_ttl_exceeded.ip_header.ip_sum = 0;
        inet_pton(AF_INET, "127.0.0.1", &m_icmp_ttl_exceeded.ip_header.ip_src);  // TODO the ip of the proxy server
        inet_pton(AF_INET, g_icmp_echo_destination.c_str(), &m_icmp_ttl_exceeded.ip_header.ip_dst);
        m_icmp_ttl_exceeded.ip_header.ip_sum = htons(get_checksum(reinterpret_cast<uint16_t*>(&m_icmp_ttl_exceeded.ip_header), sizeof(ip)));

        m_icmp_ttl_exceeded.original_icmp = g_icmp_echo;
        m_icmp_ttl_exceeded.icmp.checksum = htons(get_checksum(reinterpret_cast<uint16_t*>(&m_icmp_ttl_exceeded), sizeof(icmp_ttl_exceeded)));

        // send once (for testing purposes only) TODO
        send_icmp_ttl_exceeded();
    }


private:
    // TODO timed every 5 sec
    void send_icmp_ttl_exceeded() {
        auto buffer = boost::asio::buffer(&m_icmp_ttl_exceeded, sizeof(icmp_ttl_exceeded));
        auto callback = boost::bind(&ConnectingTCPClient::handle_send, this, boost::asio::placeholders::error);
        m_icmp_socket.async_send(buffer, callback);
    }

    void handle_send(const boost::system::error_code& error) {
        if (error) {
            cerr << "Warning: send icmp ttl exceeded failed: " << error.message() << endl;
        }
    }

private:
    boost::asio::ip::tcp::socket* m_tcp_socket;
    boost::asio::ip::icmp::socket m_icmp_socket;
     
    icmp_ttl_exceeded m_icmp_ttl_exceeded;
};
