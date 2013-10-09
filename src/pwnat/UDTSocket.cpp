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
#include <boost/array.hpp>
#include "udtservice/UDTService.h"

using namespace std;

UDTSocket::UDTSocket(UDTService& udt_service, u_int16_t source_port, u_int16_t destination_port, boost::asio::ip::address_v4 destination, NetworkPipe& pipe) :
    m_udt_service(udt_service),
    m_socket(UDT::socket(AF_INET, SOCK_STREAM, 0)),
    m_name("UDT socket"),  // TODO include src->dst port in name
    m_pipe(pipe)
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

    // TODO connecting can take a while (bind is instant), we should figure out how to do this in an async manner
    localhost.sin_addr.s_addr = htonl(destination.to_ulong());
    localhost.sin_port = htons(destination_port);
    if (UDT::ERROR == UDT::connect(m_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
        cerr << "udt connect error: " << UDT::getlasterror().getErrorMessage() << endl;
        abort();
    }

    bool non_blocking_mode = false;
    UDT::setsockopt(m_socket, 0, UDT_SNDSYN, &non_blocking_mode, sizeof(bool));
    UDT::setsockopt(m_socket, 0, UDT_RCVSYN, &non_blocking_mode, sizeof(bool));

    m_udt_service.request_receive(*this);
}

void UDTSocket::receive() {
    boost::array<char, 64 * 1024> buffer; // allocate buffer to more or less the max an IP packet can carry

    cout << "receiving" << endl;
    int bytes_transferred = UDT::recv(m_socket, buffer.data(), buffer.size(), 0);
    if (bytes_transferred == UDT::ERROR) {
        auto error = UDT::getlasterror();
        const int EASYNCRCV = 6002; // no data available to receive
        if (error.getErrorCode() != EASYNCRCV) {
            cerr << m_name << ": error: " << error.getErrorMessage() << endl;
            abort();
        }
    }
    else {
        cout << m_name << " received " << bytes_transferred << endl;
        ConstPacket packet(buffer.data(), bytes_transferred);
        m_pipe.push(packet);
    }

    // always request to receive more
    m_udt_service.request_receive(*this);
}

void UDTSocket::push(ConstPacket& packet) {
    ostream ostr(&m_buffer);
    ostr.write(packet.data(), packet.length());
    send();
}

void UDTSocket::send() {
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
        m_udt_service.request_send(*this);
    }
}

UDTSOCKET UDTSocket::socket() {
    return m_socket;
}

