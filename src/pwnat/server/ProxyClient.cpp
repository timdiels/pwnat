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
#include <pwnat/udtservice/UDTService.h>
#include <pwnat/constants.h>
#include <pwnat/packet.h>
#include "ProxyServer.h"

#include <pwnat/namespaces.h>

ProxyClient::ProxyClient(ProxyServer& server, asio::io_service& io_service, UDTService& udt_service, asio::ip::address_v4 address, u_int16_t flow_id) : 
    m_address(address),
    m_flow_id(flow_id),
    m_server(server),
    m_tcp_socket(make_shared<TCPSocket>(io_service, bind(&ProxyClient::die, this))),
    m_udt_socket(make_shared<UDTSocket>(udt_service, bind(&ProxyClient::die, this)))
{
    auto& args = Application::instance().args();

    m_udt_socket->init();
    m_udt_socket->on_received_data(bind(&ProxyClient::on_receive_udt, this, _1));
    m_udt_socket->connect(args.proxy_port(), m_address, 44401u);

    m_tcp_socket->init();

    m_udt_socket->receive_data_from(*m_tcp_socket);
}

ProxyClient::~ProxyClient() {
    m_udt_socket->dispose();
    m_tcp_socket->dispose();
    cout << "ProxyClient: Deallocated" << endl;
}

asio::ip::address_v4 ProxyClient::address() {
    return m_address;
}

u_int16_t ProxyClient::flow_id() {
    return m_flow_id;
}

void ProxyClient::die() {
    m_server.kill_client(*this);
}

// used only initially to receive the udt_flow_init
void ProxyClient::on_receive_udt(asio::streambuf& receive_buffer) {
    if (receive_buffer.size() > sizeof(udt_flow_init)) {
        auto* buffer = asio::buffer_cast<const char*>(receive_buffer.data());
        auto* flow_init = reinterpret_cast<const udt_flow_init*>(buffer);
        if (flow_init->size <= receive_buffer.size()) {
            string remote_host(buffer + sizeof(udt_flow_init), flow_init->size - sizeof(udt_flow_init));
            cout << "connecting to " << remote_host << ":" << flow_init->remote_port << endl;

            m_tcp_socket->connect(0, asio::ip::address::from_string(remote_host), flow_init->remote_port);
            receive_buffer.consume(flow_init->size);

            m_tcp_socket->receive_data_from(*m_udt_socket);  // this also unsets our on_receive handler
        }
    }
}

