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
#include <cassert>
#include <pwnat/namespaces.h>
#include "udtservice/UDTService.h"
#include "util.h"

UDTSocket::UDTSocket(UDTService& udt_service, DeathHandler death_handler) :
    AbstractSocket(false, death_handler, "UDT socket"),
    m_udt_service(udt_service),
    m_socket(UDT::socket(AF_INET, SOCK_STREAM, 0))
{
    if (m_socket == UDT::INVALID_SOCK) {
        cerr << m_name << ": Invalid UDTSOCKET" << endl;
        die();
        return; // TODO maybe we should throw instead of return
    }
}

UDTSocket::~UDTSocket() {
    UDT::close(m_socket);
}

void UDTSocket::connect(u_int16_t source_port, boost::asio::ip::address destination, u_int16_t destination_port) {
    if (disposed()) return;
    assert(!connected());
    // TODO support ipv6, everywhere

    sockaddr_in localhost;
    localhost.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &localhost.sin_addr);
    memset(&localhost.sin_zero, 0, 8);

    bool rendezvous = true;
    UDT::setsockopt(m_socket, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(bool));

    localhost.sin_port = htons(source_port);
    if (UDT::ERROR == UDT::bind(m_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
        cerr << m_name << ": bind() error: " << UDT::getlasterror().getErrorMessage() << endl;
        die();
        return;
    }

    bool non_blocking_mode = false;
    UDT::setsockopt(m_socket, 0, UDT_SNDSYN, &non_blocking_mode, sizeof(bool));
    UDT::setsockopt(m_socket, 0, UDT_RCVSYN, &non_blocking_mode, sizeof(bool));

    localhost.sin_addr.s_addr = htonl(destination.to_v4().to_ulong());
    localhost.sin_port = htons(destination_port);
    if (UDT::ERROR == UDT::connect(m_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
        cerr << m_name << ": connect() error: " << UDT::getlasterror().getErrorMessage() << endl;
        die();
        return;
    }

    // find out when we're connected
    m_udt_service.request_send(m_socket, boost::bind(&UDTSocket::notify_connected, shared_from_this()));
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
    int bytes_transferred = UDT::recv(m_socket, boost::asio::buffer_cast<char*>(m_receive_buffer.prepare(buffer_size)), buffer_size, 0);
    if (bytes_transferred == UDT::ERROR) {
        auto error = UDT::getlasterror();
        const int EASYNCRCV = 6002; // no data available to receive
        if (error.getErrorCode() != EASYNCRCV) {
            cerr << m_name << ": error: " << error.getErrorMessage() << endl;
            die();
            return;
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
    int bytes_transferred = UDT::send(m_socket, boost::asio::buffer_cast<const char*>(m_send_buffer.data()), m_send_buffer.size(), 0);
    if (bytes_transferred == UDT::ERROR) {
        cerr << m_name << ": error: " << UDT::getlasterror().getErrorMessage() << endl;
        die();
        return;
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

