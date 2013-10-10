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
#include "udtservice/UDTService.h"
#include "UDTSocket.h"
#include "constants.h"
#include <pwnat/packet.h>
#include "util.h"

using namespace std;

class UDTClient { // TODO rename ProxyClient
public:
    UDTClient(boost::asio::io_service& io_service, UDTService& udt_service, boost::asio::ip::address_v4 destination) : 
        m_tcp_socket(io_service),
        m_tcp_sender(nullptr),
        m_udt_socket(udt_service, udp_port_s, udp_port_c, destination),

        m_tcp_receiver(nullptr)
    {
        auto callback = boost::bind(&UDTClient::handle_tcp_connected, this, boost::asio::placeholders::error);
        m_tcp_socket.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 22u), callback); // TODO async when udt client connects
    }

    virtual ~UDTClient() {
        if (m_tcp_receiver) {
            delete m_tcp_receiver;
        }
        if (m_tcp_sender) {
            delete m_tcp_sender;
        }
    }

private:
    void handle_tcp_connected(boost::system::error_code error) {
        if (error) {
            cout << "tcp failed to connect" << endl;
            abort(); // TODO one day we'll have to implement more robust handling than a simple abort
            // TODO we'll also want logging of various verbosity levels
        }
        else {
            m_tcp_receiver = new TCPReceiver(m_tcp_socket, m_udt_socket, "tcp receiver");
            m_tcp_sender = new TCPSender(m_tcp_socket, "tcp sender");
            m_udt_socket.pipe(*m_tcp_sender);
        }
    }

private:
    boost::asio::ip::tcp::socket m_tcp_socket;
    TCPSender* m_tcp_sender;
    UDTSocket m_udt_socket;
    TCPReceiver* m_tcp_receiver;
};

/**
 * Listens for new UDTClients using pwnat ICMP trickery
 */
class Server { // TODO rename ProxyServer
public:
    Server() :
        m_socket(m_io_service, boost::asio::ip::icmp::endpoint(boost::asio::ip::icmp::v4(), 0)),
        m_udt_service(m_io_service)
    {
        m_socket.connect(boost::asio::ip::icmp::endpoint(boost::asio::ip::address::from_string(g_icmp_echo_destination), 0));
        send_icmp_echo();
        start_receive();
    }

    void run() {
        cout << "running server" << endl;
        m_io_service.run();
    }

private:
    // TODO timed every 5 sec
    void send_icmp_echo() {
        auto buffer = boost::asio::buffer(&g_icmp_echo, sizeof(g_icmp_echo));
        auto callback = boost::bind(&Server::handle_send, this, boost::asio::placeholders::error);
        m_socket.async_send(buffer, callback);
    }

    void handle_send(const boost::system::error_code& error) {
        if (error) {
            cerr << "Warning: send icmp echo failed: " << error.message() << endl;
        }
    }

    void start_receive() {
        auto callback = boost::bind(&Server::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        m_socket.async_receive(boost::asio::buffer(m_receive_buffer), callback);
    }

    void handle_receive(boost::system::error_code error, size_t bytes_transferred) {
        if (error) {
            cerr << "Warning: icmp receive error: " << error.message() << endl;
        }
        else {
            cout << "received icmp" << endl;
            auto ip_header = reinterpret_cast<ip*>(m_receive_buffer.data());
            const int ip_header_size = ip_header->ip_hl * 4;
            auto header = reinterpret_cast<icmp_ttl_exceeded*>(m_receive_buffer.data() + ip_header_size);

            if (bytes_transferred == ip_header_size + sizeof(icmp_ttl_exceeded) &&
                header->ip_header.ip_hl == 5 &&
                header->icmp.type == ICMP_TIME_EXCEEDED &&
                memcmp(&header->original_icmp, &g_icmp_echo, sizeof(g_icmp_echo)) == 0) 
            {
                // somebody wants to connect, so let them
                cout << "Accepting new proxy client" << endl;
                boost::asio::ip::address_v4 destination(ntohl(ip_header->ip_src.s_addr));
                new UDTClient(m_io_service, m_udt_service, destination); // TODO shouldn't do this more than once! (keep a map of ip -> client)
            }
        }

        start_receive();
    }

private:
    boost::asio::io_service m_io_service;
    boost::asio::ip::icmp::socket m_socket;
    boost::array<char, 64 * 1024> m_receive_buffer;

    UDTService m_udt_service;
};

