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

#include "ProxyServer.h"
#include <boost/bind.hpp>
#include <cassert>
#include <pwnat/UDTSocket.h>
#include <pwnat/constants.h>
#include "ProxyClient.h"
#include <pwnat/packet.h>

#include <pwnat/namespaces.h>

ProxyServer::ProxyServer() :
    m_socket(m_io_service, asio::ip::icmp::endpoint(asio::ip::icmp::v4(), 0)),
    m_icmp_timer(m_io_service)
{
    m_socket.connect(asio::ip::icmp::endpoint(asio::ip::address::from_string(g_icmp_echo_destination), 0));
    send_icmp_echo();
    start_receive();
}

ProxyServer::~ProxyServer() {
    for (auto entry : m_clients) {
        delete entry.second;
    }
}

void ProxyServer::send_icmp_echo() {
    // send
    {
        auto buffer = asio::buffer(&g_icmp_echo, sizeof(g_icmp_echo));
        auto callback = bind(&ProxyServer::handle_send, this, asio::placeholders::error);
        m_socket.async_send(buffer, callback);
    }

    // set timer
    {
        m_icmp_timer.expires_from_now(boost::posix_time::seconds(5));
        auto callback = bind(&ProxyServer::handle_icmp_timer_expired, this, asio::placeholders::error);
        m_icmp_timer.async_wait(callback);
    }
}

void ProxyServer::handle_icmp_timer_expired(const boost::system::error_code& error) {
    if (error) {
        if (error.value() != asio::error::operation_aborted) {  // aborted = timer cancelled
            cerr << "Warning: Unexpected timer error: " << error.message() << endl;
        }
    }
    else {
        send_icmp_echo();
    } 
}

void ProxyServer::handle_send(const boost::system::error_code& error) {
    if (error) {
        cerr << "Warning: send icmp echo failed: " << error.message() << endl;
    }
}

void ProxyServer::start_receive() {
    auto callback = bind(&ProxyServer::handle_receive, this, asio::placeholders::error, asio::placeholders::bytes_transferred);
    m_socket.async_receive(asio::buffer(m_receive_buffer), callback);
}

void ProxyServer::handle_receive(boost::system::error_code error, size_t bytes_transferred) {
    if (error) {
        cerr << "Warning: icmp receive error: " << error.message() << endl;
    }
    else {
        auto ip_header = reinterpret_cast<ip*>(m_receive_buffer.data());
        const int ip_header_size = ip_header->ip_hl * 4;
        auto header = reinterpret_cast<icmp_ttl_exceeded*>(m_receive_buffer.data() + ip_header_size);

        if (bytes_transferred == ip_header_size + sizeof(icmp_ttl_exceeded) &&
            header->ip_header.ip_hl == 5 &&
            header->icmp.type == ICMP_TIME_EXCEEDED &&
            memcmp(&header->original_icmp, &g_icmp_echo, sizeof(g_icmp_echo)) == 0) 
        {
            asio::ip::address_v4 client_address(ntohl(ip_header->ip_src.s_addr));
            u_int16_t flow_id = ntohs(header->ip_header.ip_id);  // we've abused the ip_id field to store the flow id in
            auto key = make_pair(client_address, flow_id);
            if (m_clients.find(key) == m_clients.end()) {
                cout << "Accepting new proxy client" << endl;
                try {
                m_clients[key] = new ProxyClient(*this, m_io_service, m_udt_service, client_address, flow_id);
                }
                catch (const exception& e) {
                    cerr << "Failed to create client: " << e.what() << endl;
                }
                catch (...) {
                    cerr << "Failed to create client: unknown error" << endl;
                }
            }
        }
    }

    start_receive();
}

void ProxyServer::kill_client(ProxyClient& client) {
    m_clients.erase(make_pair(client.address(), client.flow_id()));
    delete &client;
}
