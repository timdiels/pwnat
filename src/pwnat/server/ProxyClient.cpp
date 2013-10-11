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

#include "ProxyClient.h"
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <pwnat/udtservice/UDTService.h>
#include <pwnat/constants.h>
#include "ProxyServer.h"

using namespace std;

ProxyClient::ProxyClient(ProxyServer& server, boost::asio::io_service& io_service, UDTService& udt_service, boost::asio::ip::address_v4 address, u_int16_t flow_id) : 
    m_address(address),
    m_flow_id(flow_id),
    m_server(server),
    m_tcp_socket_(io_service),
    m_udt_socket(make_shared<UDTSocket>(udt_service, udp_port_s, udp_port_c, m_address, boost::bind(&ProxyClient::die, this)))
{
    auto callback = boost::bind(&ProxyClient::handle_tcp_connected, this, boost::asio::placeholders::error);
    m_tcp_socket_.async_connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 22u), callback);

    m_udt_socket->init();
}

ProxyClient::~ProxyClient() {
    cerr << "ProxyClient deallocating" << endl;
    m_udt_socket->dispose();
    m_tcp_socket->dispose();
}

boost::asio::ip::address_v4 ProxyClient::address() {
    return m_address;
}

u_int16_t ProxyClient::flow_id() {
    return m_flow_id;
}

void ProxyClient::die() {
    m_server.kill_client(*this);
}

void ProxyClient::handle_tcp_connected(boost::system::error_code error) {
    if (error) {
        cerr << "tcp socket failed to connect: " << error.message() << endl;
        die();
        // TODO one day we'll have to implement more robust handling than a simple abort
        // TODO we'll also want logging of various verbosity levels
    }
    else {
        m_tcp_socket = make_shared<TCPSocket>(m_tcp_socket_, m_udt_socket, boost::bind(&ProxyClient::die, this));
        m_tcp_socket->init();
        m_udt_socket->pipe(m_tcp_socket);
    }
}
