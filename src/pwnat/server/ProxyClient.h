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

// TODO test with dns names instead of ips

class UDTService;
class ProxyServer;

class ProxyClient {
public:
    ProxyClient(ProxyServer&, boost::asio::io_service& io_service, UDTService& udt_service, boost::asio::ip::address_v4 client_address, u_int16_t flow_id);
    virtual ~ProxyClient();

    /*
     * Get client address
     */
    boost::asio::ip::address_v4 address();

    u_int16_t flow_id();

private:
    void die();
    void handle_tcp_connected(boost::system::error_code error);

private:
    boost::asio::ip::address_v4 m_address;
    u_int16_t m_flow_id;
    ProxyServer& m_server;
    boost::asio::ip::tcp::socket m_tcp_socket_;
    std::shared_ptr<TCPSocket> m_tcp_socket;
    std::shared_ptr<UDTSocket> m_udt_socket;
};

