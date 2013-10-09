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

// TODO slim down includes to essentials
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <cassert>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
//#include "socket.h" // TODO rename file: util.h
#include "packet.h" // TODO rename to checksum.h
#include "udtservice/UDTService.h"
#include "UDTSocket.h"
#include "Socket.h"
#include "constants.h"

using namespace std;

class TCPClient {
public:
    TCPClient(UDTService& udt_service, boost::asio::ip::tcp::socket* tcp_socket) :
        m_tcp_socket(tcp_socket),

        m_tcp_sender(*m_tcp_socket, "tcp sender"),
        m_udt_socket(udt_service, udp_port_c, udp_port_s, m_tcp_sender),

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

class ClientTCPServer {
public:
    ClientTCPServer(boost::asio::io_service& io_service) :
        m_udt_service(io_service),
        m_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 44401u))
    {
        accept();
    }

    void accept() {
        auto new_socket = new boost::asio::ip::tcp::socket(m_acceptor.get_io_service());
        auto callback = boost::bind(&ClientTCPServer::handle_accept, this, boost::asio::placeholders::error, new_socket);
        m_acceptor.async_accept(*new_socket, callback);
    }

    void handle_accept(const boost::system::error_code& error, boost::asio::ip::tcp::socket* tcp_socket) {
        if (error) {
            cerr << "TCP Server: accept error: " << error.message() << endl;
        }
        else {
            cout << "New tcp client at port " << tcp_socket->remote_endpoint().port() << endl;
            new TCPClient(m_udt_service, tcp_socket);  // Note: ownership of socket transferred to TCPClient instance
        }

        accept();
    }

private:
    UDTService m_udt_service;
    boost::asio::ip::tcp::acceptor m_acceptor;
};

class Client {
public:
    Client() :
        m_tcp_server(m_io_service)
    {
    }

    void run() {
        cout << "running client" << endl;
        m_io_service.run();
    }

private:
    boost::asio::io_service m_io_service;
    ClientTCPServer m_tcp_server;
};

