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
#include <boost/asio/spawn.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <cassert>
#include <netinet/ip_icmp.h>
#include "packet.h" // TODO rename to checksum.h
#include "udtservice/UDTService.h"
#include "UDTSocket.h"
#include "constants.h"

using namespace std;

class UDTClient {
public:
    UDTClient(boost::asio::io_service& io_service, UDTService& udt_service) : 
        m_tcp_socket(io_service),
        m_tcp_sender(m_tcp_socket, "tcp sender"),
        m_udt_socket(udt_service, udp_port_s, udp_port_c, m_tcp_sender),

        m_tcp_receiver(m_tcp_socket, m_udt_socket, "tcp receiver")
    {
        m_tcp_socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 22u)); // TODO async when udt client connects
    }

private:
    boost::asio::ip::tcp::socket m_tcp_socket;
    TCPSender m_tcp_sender;
    UDTSocket m_udt_socket;
    TCPReceiver m_tcp_receiver;
};

/**
 * Listens for new UDTClients using pwnat ICMP trickery
 */
class Server { // TODO rename to UDTServer or ProxyServer
public:
    Server() :
        m_socket(m_io_service, boost::asio::ip::icmp::endpoint(boost::asio::ip::icmp::v4(), 0)),
        m_udt_service(m_io_service)
    {
        m_socket.connect(boost::asio::ip::icmp::endpoint(boost::asio::ip::address::from_string(g_icmp_address), 0));

        m_icmp_request.type = ICMP_ECHO;
        m_icmp_request.code = 0;
        m_icmp_request.checksum = 0;
        m_icmp_request.un.echo.id = 0;
        m_icmp_request.un.echo.sequence = 0;
        m_icmp_request.checksum = htons(0xf7ff);

        send_icmp_request();
    }

    void run() {
        cout << "running server" << endl;
        m_io_service.run();
    }

private:
    // TODO timed every 5 sec
    void send_icmp_request() {
        auto buffer = boost::asio::buffer(&m_icmp_request, sizeof(icmphdr));
        auto callback = boost::bind(&Server::handle_send, this, boost::asio::placeholders::error);
        m_socket.async_send(buffer, callback);
    }

    void handle_send(const boost::system::error_code& error) {
        if (error) {
            cerr << "Warning: ping failed: " << error.message() << endl;
        }
    }

private:
    boost::asio::io_service m_io_service;
    boost::asio::ip::icmp::socket m_socket;
    icmphdr m_icmp_request;

    UDTService m_udt_service;
};

