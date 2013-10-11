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

#include "Socket.h"
#include <boost/bind.hpp>

using namespace std;
using boost::bind;

template<typename SocketType>
Socket<SocketType>::Socket(SocketType& socket, DeathHandler death_handler) : 
    AbstractSocket(true, death_handler, "tcp socket"),
    m_socket(socket)
{
}

template<typename SocketType>
Socket<SocketType>::Socket(DeathHandler death_handler) : 
    AbstractSocket(true, death_handler, "tcp socket")
{
}

template<typename SocketType>
void Socket<SocketType>::connect(u_int16_t source_port, boost::asio::ip::address destination, u_int16_t destination_port) {
    if (disposed()) return;
    assert(!connected());
    // TODO
}

template<typename SocketType>
void Socket<SocketType>::receive_data_from(AbstractSocket& socket) {
    socket.on_received_data(bind(&Socket<SocketType>::send, this->shared_from_this(), _1));
}

template<typename SocketType>
void Socket<SocketType>::start_receiving() {
    if (disposed()) return;
    auto callback = boost::bind(&Socket::handle_receive, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
    m_socket.async_receive(boost::asio::buffer(m_receive_buffer.prepare(64 * 1024)), callback);
}

template<typename SocketType>
void Socket<SocketType>::start_sending() {
    if (disposed()) return;
    if (m_send_buffer.size() > 0) {
        auto callback = boost::bind(&Socket::handle_send, this->shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        m_socket.async_send(m_send_buffer.data(), callback);
    }
}

template<typename SocketType>
void Socket<SocketType>::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
    if (disposed()) return;
    if (error) {
        cerr << m_name << ": receive error: " << error.message() << endl;
        die();
        return;
    }
    else {
        cout << m_name << " received " << bytes_transferred << endl;
        m_receive_buffer.commit(bytes_transferred);
        notify_received_data();
    }

    start_receiving();
}

template<typename SocketType>
void Socket<SocketType>::handle_send(const boost::system::error_code& error, size_t bytes_transferred) {
    if (disposed() || !connected() || m_send_buffer.size() == 0) return;

    if (error) {
        cerr << m_name << ": send error: " << error.message() << endl;
        die();
        return;
    }
    else {
        cout << m_name << " sent " << bytes_transferred << endl;
        m_send_buffer.consume(bytes_transferred);
        send();
    }
}

template<typename SocketType>
SocketType& Socket<SocketType>::socket() {
    return m_socket;
}

template class Socket<boost::asio::ip::tcp::socket>;
