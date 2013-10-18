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

#include "UDTSocket.h"
#include <sstream>
#include <cassert>
#include <pwnat/util.h>
#include <pwnat/udtservice/UDTService.h>
#include <pwnat/Application.h>

#include <pwnat/namespaces.h>

UDTSocket::UDTSocket(UDTService& udt_service, DeathHandler death_handler) :
    AbstractSocket(false, death_handler, "UDT socket"),
    m_udt_service(udt_service),
    m_socket(UDT::socket(Application::instance().args().address_family(), SOCK_STREAM, 0))
{
    if (m_socket == UDT::INVALID_SOCK) {
        die(format_udt_error("Could not create UDTSOCKET"));
    }
}

UDTSocket::~UDTSocket() {
    UDT::close(m_socket);
}

void UDTSocket::connect(u_int16_t source_port, asio::ip::address destination, u_int16_t destination_port) {
    if (disposed()) return;
    assert(!connected());

    auto& args = Application::instance().args();

    vector<char> source_addr;
    vector<char> dest_addr;
    if (args.is_ipv6()) {
        source_addr.resize(sizeof(sockaddr_in6), 0);

        auto addr = reinterpret_cast<sockaddr_in6*>(source_addr.data());
        addr->sin6_family = args.address_family();
        addr->sin6_port = htons(source_port);
        inet_pton(args.address_family(), args.bind_address().to_string().c_str(), &addr->sin6_addr);

        dest_addr = source_addr;
        addr = reinterpret_cast<sockaddr_in6*>(dest_addr.data());
        addr->sin6_port = htons(destination_port);
        inet_pton(args.address_family(), destination.to_string().c_str(), &addr->sin6_addr);
    }
    else {
        source_addr.resize(sizeof(sockaddr_in), 0);

        auto addr = reinterpret_cast<sockaddr_in*>(source_addr.data());
        addr->sin_family = args.address_family();
        addr->sin_port = htons(source_port);
        inet_pton(args.address_family(), args.bind_address().to_string().c_str(), &addr->sin_addr);

        dest_addr = source_addr;
        addr = reinterpret_cast<sockaddr_in*>(dest_addr.data());
        addr->sin_port = htons(destination_port);
        inet_pton(args.address_family(), destination.to_string().c_str(), &addr->sin_addr);
    }

    bool rendezvous = true;
    UDT::setsockopt(m_socket, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(bool));

    if (UDT::ERROR == UDT::bind(m_socket, reinterpret_cast<sockaddr*>(source_addr.data()), source_addr.size())) {
        die(format_udt_error("Could not bind"));
    }

    bool non_blocking_mode = false;
    UDT::setsockopt(m_socket, 0, UDT_SNDSYN, &non_blocking_mode, sizeof(bool));
    UDT::setsockopt(m_socket, 0, UDT_RCVSYN, &non_blocking_mode, sizeof(bool));

    if (UDT::ERROR == UDT::connect(m_socket, reinterpret_cast<sockaddr*>(dest_addr.data()), dest_addr.size())) {
        die(format_udt_error("Could not connect"));
    }

    // find out when we're connected
    m_udt_service.request_send(m_socket, bind(&UDTSocket::notify_connected, shared_from_this()));
}

void UDTSocket::receive_data_from(AbstractSocket& socket) {
    socket.on_received_data(bind(&UDTSocket::send, shared_from_this(), _1));
}

bool UDTSocket::dispose() {
    if (AbstractSocket::dispose()) {
        m_udt_service.request_unregister(m_socket);
        return true;
    }
    else {
        return false;
    }
}

void UDTSocket::start_receiving() {
    assert(connected());
    m_udt_service.request_receive(m_socket, bind(&UDTSocket::handle_receive, shared_from_this()));
}

void UDTSocket::start_sending() {
    assert(connected());
    m_udt_service.request_send(m_socket, bind(&UDTSocket::handle_send, shared_from_this()));
}

void UDTSocket::handle_receive() {
    if (disposed()) return;

    cout << "receiving" << endl;
    const size_t buffer_size = 64 * 1024;
    int bytes_transferred = UDT::recv(m_socket, asio::buffer_cast<char*>(m_receive_buffer.prepare(buffer_size)), buffer_size, 0);
    if (bytes_transferred == UDT::ERROR) {
        auto error = UDT::getlasterror();
        const int EASYNCRCV = 6002; // no data available to receive
        if (error.getErrorCode() != EASYNCRCV) {
            die(format_udt_error("Failed to receive"));
        }
    }
    else {
        cout << m_name << " received " << bytes_transferred << endl;
        m_receive_buffer.commit(bytes_transferred);
        print_hexdump(asio::buffer_cast<const char*>(m_receive_buffer.data()), m_receive_buffer.size());
        notify_received_data();
    }

    start_receiving();
}

void UDTSocket::handle_send() {
    if (disposed() || !connected() || m_send_buffer.size() == 0) return;

    cout << "sending " << m_send_buffer.size() << endl;
    print_hexdump(asio::buffer_cast<const char*>(m_send_buffer.data()), m_send_buffer.size());
    int bytes_transferred = UDT::send(m_socket, asio::buffer_cast<const char*>(m_send_buffer.data()), m_send_buffer.size(), 0);
    if (bytes_transferred == UDT::ERROR) {
        die(format_udt_error("Failed to send"));
    }
    else {
        cout << m_name << " sent " << bytes_transferred << endl;
        m_send_buffer.consume(bytes_transferred);
    }

    if (m_send_buffer.size() > 0) {
        // didn't manage to send everything, assume internal buffer is full
        cout << m_name << ": send buffer full, waiting" << endl;
        start_sending();
    }
}

