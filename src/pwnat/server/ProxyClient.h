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

#include "ProxyClient.h"
#include <boost/asio.hpp>
#include <pwnat/UDTSocket.h>
#include <pwnat/Socket.h>

class UDTService;
class ProxyServer;

class ProxyClient {
public:
    // uniquely identifies a proxy client
    class Id {
    public:
        bool operator== (const Id& b) const {
            return flow_id == b.flow_id && address == b.address && client_port == b.client_port;
        }

        bool operator> (const Id& b) const {
            if (flow_id > b.flow_id)
                return true;
            else if (address > b.address)
                return true;
            else if (client_port > b.client_port)
                return true;
            else
                return false;
        }

        bool operator<= (const Id& b) const {
            return *this == b || *this < b;
        }

        bool operator< (const Id& b) const {
            return !(*this >= b);
        }

        bool operator>= (const Id& b) const {
            return !(*this < b);
        }

    public:
        boost::asio::ip::address address;
        u_int16_t flow_id;
        u_int16_t client_port;
    };

public:
    ProxyClient(ProxyServer&, boost::asio::io_service& io_service, UDTService& udt_service, ProxyClient::Id client_id);
    virtual ~ProxyClient();

    const Id& id();

private:
    void die();
    void on_receive_udt(boost::asio::streambuf& receive_buffer);
    void on_resolved_remote_host(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator result);

private:
    Id m_id;
    boost::asio::io_service& m_io_service;
    ProxyServer& m_server;
    std::shared_ptr<TCPSocket> m_tcp_socket;
    std::shared_ptr<UDTSocket> m_udt_socket;
    boost::asio::ip::tcp::resolver m_resolver;
};

