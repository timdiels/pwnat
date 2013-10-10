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
#include <boost/array.hpp>
#include "udtservice/UDTService.h"

using namespace std;

UDTSocket::UDTSocket(UDTService& udt_service, u_int16_t source_port, u_int16_t destination_port, boost::asio::ip::address_v4 destination) :
    m_udt_service(udt_service),
    m_socket(UDT::socket(AF_INET, SOCK_STREAM, 0)),
    m_name("UDT socket"),  // TODO include src->dst port in name
    m_connected(false),
    m_pipe(nullptr)
{
    assert(m_socket != UDT::INVALID_SOCK); // TODO exception throw

    sockaddr_in localhost;
    localhost.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &localhost.sin_addr);
    memset(&localhost.sin_zero, 0, 8);

    bool rendezvous = true;
    UDT::setsockopt(m_socket, 0, UDT_RENDEZVOUS, &rendezvous, sizeof(bool));

    localhost.sin_port = htons(source_port);
    if (UDT::ERROR == UDT::bind(m_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
        cerr << "udt bind error: " << UDT::getlasterror().getErrorMessage() << endl;
        abort();
    }

    bool non_blocking_mode = false;
    UDT::setsockopt(m_socket, 0, UDT_SNDSYN, &non_blocking_mode, sizeof(bool));
    UDT::setsockopt(m_socket, 0, UDT_RCVSYN, &non_blocking_mode, sizeof(bool));

    localhost.sin_addr.s_addr = htonl(destination.to_ulong());
    localhost.sin_port = htons(destination_port);
    if (UDT::ERROR == UDT::connect(m_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
        cerr << "udt connect error: " << UDT::getlasterror().getErrorMessage() << endl;
        abort();
    }
}

UDTSocket::~UDTSocket() {
    assert(disposed());
    UDT::close(m_socket);
    cout << m_name << " closed" << endl;
}

void UDTSocket::dispose() {
    if (Disposable::dispose()) {
        m_udt_service.request_unregister(m_socket);
    }
}

void UDTSocket::init() {
    request_receive();
    request_send(); // this is to find out when we're connected
}

void UDTSocket::request_receive() {
    m_udt_service.request_receive(m_socket, boost::bind(&UDTSocket::receive, shared_from_this()));
}

void UDTSocket::request_send() {
    m_udt_service.request_send(m_socket, boost::bind(&UDTSocket::send, shared_from_this()));
}

void UDTSocket::receive() {
    if (disposed() || !m_pipe) {
        return;
    }

    boost::array<char, 64 * 1024> buffer; // allocate buffer to more or less the max an IP packet can carry

    cout << "receiving" << endl;
    int bytes_transferred = UDT::recv(m_socket, buffer.data(), buffer.size(), 0);
    if (bytes_transferred == UDT::ERROR) {
        auto error = UDT::getlasterror();
        const int EASYNCRCV = 6002; // no data available to receive
        if (error.getErrorCode() != EASYNCRCV) {
            cerr << m_name << ": error: " << error.getErrorMessage() << endl;
            abort(); // TODO die() instead
        }
    }
    else {
        cout << m_name << " received " << bytes_transferred << endl;
        ConstPacket packet(buffer.data(), bytes_transferred);
        m_pipe->push(packet);
    }

    // always request to receive more
    request_receive();
}

void UDTSocket::push(ConstPacket& packet) {
    if (disposed()) {
        return;
    }

    ostream ostr(&m_buffer);
    ostr.write(packet.data(), packet.length());
    if (m_connected) {
        send();
    }
}

void UDTSocket::add_connection_listener(boost::function<void()> handler) {
    if (disposed()) {
        return;
    }

    if (m_connected) {
        handler();
    }
    else {
        m_connection_listeners.push_back(handler);
    }
}

void UDTSocket::pipe(NetworkPipe& pipe) {
    if (disposed()) {
        return;
    }

    m_pipe = &pipe;
    request_receive();
}

void UDTSocket::send() {
    if (disposed()) {
        return;
    }

    if (!m_connected) {
        m_connected = true;
        for (auto handler : m_connection_listeners) {
            handler();
        }
        m_connection_listeners.clear();
    }

    if (m_buffer.size() == 0) {
        return;
    }

    cout << "sending " << m_buffer.size() << endl;
    int bytes_transferred = UDT::send(m_socket, boost::asio::buffer_cast<const char*>(m_buffer.data()), m_buffer.size(), 0);
    if (bytes_transferred == UDT::ERROR) {
        cerr << m_name << ": error: " << UDT::getlasterror().getErrorMessage() << endl;
        abort();
    }
    else {
        cout << m_name << " sent " << bytes_transferred << endl;
        m_buffer.consume(bytes_transferred);
    }

    if (m_buffer.size() > 0) {
        // didn't manage to send everything, assume internal buffer is full
        cout << m_name << ": send buffer full, waiting" << endl;
        request_send();
    }
}

UDTSOCKET UDTSocket::socket() {
    return m_socket;
}

