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
#include <pwnat/UDTSocket.h>
#include <pwnat/Socket.h>
#include <pwnat/packet.h>

class UDTService;

class TCPClient {
public:
    TCPClient(UDTService& udt_service, boost::asio::ip::tcp::socket* tcp_socket);
    ~TCPClient();

private:
    void die();
    void build_icmp_ttl_exceeded();
    void send_icmp_ttl_exceeded();
    void handle_send(const boost::system::error_code& error);
    void handle_icmp_timer_expired(const boost::system::error_code& error);
    void handle_connected();

private:
    UDTSocket m_udt_socket;
    TCPSocket m_tcp_socket;

    boost::asio::ip::icmp::socket m_icmp_socket;
    boost::asio::deadline_timer m_icmp_timer;
    icmp_ttl_exceeded m_icmp_ttl_exceeded;
};
