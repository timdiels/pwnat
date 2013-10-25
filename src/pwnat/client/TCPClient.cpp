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

#include "TCPClient.h"
#include <boost/bind.hpp>
#include <pwnat/udtservice/UDTService.h>
#include <pwnat/checksum.h>
#include <pwnat/Application.h>
#include <boost/log/trivial.hpp>

#include <pwnat/namespaces.h>

TCPClient::TCPClient(UDTService& udt_service, asio::ip::tcp::socket* tcp_socket, u_int16_t flow_id) :
    m_udt_socket(make_shared<UDTSocket>(udt_service, bind(&TCPClient::die, this))),
    m_tcp_socket(make_shared<TCPSocket>(shared_ptr<asio::ip::tcp::socket>(tcp_socket), bind(&TCPClient::die, this))), 
    m_icmp_socket(tcp_socket->get_io_service(), asio::ip::icmp::endpoint(Application::instance().args().icmp_version(), 0)),
    m_icmp_timer(tcp_socket->get_io_service())
{
    auto& args = Application::instance().args();

    m_udt_socket->init();
    send_udt_flow_init(args.remote_host(), args.remote_port()); // this must be the first data sent onto the socket
    m_udt_socket->connect(0, args.proxy_host(), args.proxy_port()); // TODO search for AF_INIT, v4
    m_udt_socket->on_connected(bind(&TCPClient::handle_udt_connected, this));

    m_tcp_socket->init();

    m_udt_socket->receive_data_from(*m_tcp_socket);
    m_tcp_socket->receive_data_from(*m_udt_socket);

    m_icmp_socket.connect(asio::ip::icmp::endpoint(args.proxy_host(), 0u));
    build_icmp_ttl_exceeded(flow_id, m_udt_socket->local_port());
    send_icmp_ttl_exceeded();
    // TODO multiple TCPClients cause segfault in pwnat server
}

TCPClient::~TCPClient() {
    BOOST_LOG_TRIVIAL(debug) << "TCPClient: Deallocated" << endl;
}

void TCPClient::die() {
    m_udt_socket->dispose();
    m_tcp_socket->dispose();
    delete this;
}

void TCPClient::send_udt_flow_init(string remote_host, u_int16_t remote_port) {
    u_int16_t size = sizeof(udt_flow_init) + remote_host.length();
    vector<char> buffer(size);
    udt_flow_init& flow_init = *reinterpret_cast<udt_flow_init*>(buffer.data());
    flow_init.size = size;
    flow_init.remote_port = remote_port;
    memcpy(buffer.data() + sizeof(udt_flow_init), remote_host.data(), remote_host.length());
    m_udt_socket->send(buffer.data(), buffer.size());
}

void TCPClient::build_icmp_ttl_exceeded(u_int16_t flow_id, u_int16_t client_port) {
    auto& args = Application::instance().args();

    vector<char> original_icmp;
    args.get_icmp_echo(original_icmp, flow_id, client_port);

    if (args.is_ipv6()) {
        m_icmp_ttl_exceeded.resize(sizeof(icmp6_ttl_exceeded), 0);
        auto icmp = reinterpret_cast<icmp6_ttl_exceeded*>(m_icmp_ttl_exceeded.data());

        icmp->icmp.icmp6_type = ICMP6_TIME_EXCEEDED;

        *reinterpret_cast<char*>(&icmp->ip_header.ip6_flow) = 0x60;  // set ip version to 6
        icmp->ip_header.ip6_plen = htons(sizeof(ip6_hdr) + sizeof(icmp6_hdr));
        icmp->ip_header.ip6_hlim = 1u;
        icmp->ip_header.ip6_nxt = IPPROTO_ICMPV6;
        inet_pton(args.address_family(), args.proxy_host().to_string().c_str(), &icmp->ip_header.ip6_src);
        inet_pton(args.address_family(), args.icmp_echo_destination().to_string().c_str(), &icmp->ip_header.ip6_dst);

        memcpy(&icmp->original_icmp, original_icmp.data(), original_icmp.size());

        // Note: icmp->icmp.icmp6_cksum is calculated for us by the OS
    }
    else {
        m_icmp_ttl_exceeded.resize(sizeof(icmp_ttl_exceeded), 0);
        auto icmp = reinterpret_cast<icmp_ttl_exceeded*>(m_icmp_ttl_exceeded.data());

        icmp->icmp.type = ICMP_TIME_EXCEEDED;

        icmp->ip_header.ip_hl = 5u;
        icmp->ip_header.ip_v = 4u;
        icmp->ip_header.ip_len = htons(sizeof(ip) + sizeof(icmphdr));
        icmp->ip_header.ip_off = IP_DF;  // set don't fragment flag (is more realistic)
        icmp->ip_header.ip_ttl = 1u;
        icmp->ip_header.ip_p = IPPROTO_ICMP;
        inet_pton(args.address_family(), args.proxy_host().to_string().c_str(), &icmp->ip_header.ip_src);
        inet_pton(args.address_family(), args.icmp_echo_destination().to_string().c_str(), &icmp->ip_header.ip_dst);
        icmp->ip_header.ip_sum = htons(get_checksum(reinterpret_cast<uint16_t*>(&icmp->ip_header), sizeof(ip)));

        memcpy(&icmp->original_icmp, original_icmp.data(), original_icmp.size());

        icmp->icmp.checksum = htons(get_checksum(reinterpret_cast<uint16_t*>(m_icmp_ttl_exceeded.data()), m_icmp_ttl_exceeded.size()));
    }
}

void TCPClient::send_icmp_ttl_exceeded() {
    // send
    {
        auto buffer = asio::buffer(m_icmp_ttl_exceeded);
        auto callback = bind(&TCPClient::handle_send, this, asio::placeholders::error);
        m_icmp_socket.async_send(buffer, callback);
    }

    // set timer
    {
        m_icmp_timer.expires_from_now(boost::posix_time::seconds(5));
        auto callback = bind(&TCPClient::handle_icmp_timer_expired, this, asio::placeholders::error);
        m_icmp_timer.async_wait(callback);
    }
}

void TCPClient::handle_icmp_timer_expired(const boost::system::error_code& error) {
    if (error) {
        if (error.value() != asio::error::operation_aborted) {  // aborted = timer cancelled
            BOOST_LOG_TRIVIAL(warning) << "Unexpected timer error: " << error.message() << endl;
            die();
        }
    }
    else {
        send_icmp_ttl_exceeded();
    } 
}

void TCPClient::handle_send(const boost::system::error_code& error) {
    if (error) {
        BOOST_LOG_TRIVIAL(warning) << "Warning: send icmp ttl exceeded failed: " << error.message() << endl;
    }
}

void TCPClient::handle_udt_connected() {
    m_icmp_timer.cancel();
}
