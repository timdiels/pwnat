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
#include <pwnat/udtservice/UDTService.h>
#include <pwnat/UDTSocket.h>
#include <pwnat/Socket.h>
#include <pwnat/constants.h>

using namespace std;

/**
 * A fully connected TCPClient
 */
class TCPClient {
public:
    TCPClient(UDTService& udt_service, boost::asio::ip::tcp::socket* tcp_socket) :
        m_tcp_socket(tcp_socket),

        m_tcp_sender(*m_tcp_socket, "tcp sender"),
        m_udt_socket(udt_service, udp_port_c, udp_port_s),

        m_tcp_receiver(*m_tcp_socket, m_udt_socket, "tcp receiver") // TODO name + tcp client port, or no names, or name defaults to: proto sender/receiver src->dst port (let's do that latter)
    {
    }

    ~TCPClient() {
        delete m_tcp_socket;
    }

private:
    boost::asio::ip::tcp::socket* m_tcp_socket;
    TCPSender m_tcp_sender;
    UDTSocket m_udt_socket;
    TCPReceiver m_tcp_receiver;
};

