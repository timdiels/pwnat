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
#include <pwnat/namespaces.h>

template<typename SocketType>
Socket<SocketType>::Socket(shared_ptr<SocketType> socket, DeathHandler death_handler) : 
    AbstractSocket(true, death_handler, "tcp socket"),
    m_socket(socket),
    m_receiving(false),
    m_sending(false)
{
}

template<typename SocketType>
Socket<SocketType>::Socket(asio::io_service& io_service, DeathHandler death_handler) : 
    AbstractSocket(false, death_handler, "tcp socket"),
    m_socket(make_shared<SocketType>(io_service))
{
}

template<typename SocketType>
void Socket<SocketType>::connect(u_int16_t source_port, asio::ip::address destination, u_int16_t destination_port) {
    if (disposed()) return;
    assert(!connected());
    assert(source_port == 0); // Note: custom source_port not supported by this class due to laziness

    auto callback = bind(&Socket<SocketType>::handle_connected, this->shared_from_this(), asio::placeholders::error);
    m_socket->async_connect(asio::ip::tcp::endpoint(destination, destination_port), callback);
}

template<typename SocketType>
void Socket<SocketType>::handle_connected(boost::system::error_code error) {
    if (error) {
        cerr << "tcp socket failed to connect: " << error.message() << endl;
        die();
    }
    else {
        notify_connected();
    }
}

// TODO one day we'll have to implement more robust handling than a simple abort
// TODO we'll also want logging of various verbosity levels

template<typename SocketType>
void Socket<SocketType>::receive_data_from(AbstractSocket& socket) {
    socket.on_received_data(bind(&Socket<SocketType>::send, this->shared_from_this(), _1));
}

template<typename SocketType>
void Socket<SocketType>::start_receiving() {
    if (disposed()) return;
    if (!m_receiving) {
        m_receiving = true;
        auto callback = bind(&Socket::handle_receive, this->shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred);
        m_socket->async_receive(asio::buffer(m_receive_buffer.prepare(64 * 1024)), callback);
    }
}

template<typename SocketType>
void Socket<SocketType>::start_sending() {
    if (disposed()) return;
    if (!m_sending && m_send_buffer.size() > 0) {
        m_sending = true;
        auto callback = bind(&Socket::handle_send, this->shared_from_this(), asio::placeholders::error, asio::placeholders::bytes_transferred);
        m_socket->async_send(m_send_buffer.data(), callback);
    }
}

template<typename SocketType>
void Socket<SocketType>::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
    if (disposed()) return;

    m_receiving = false;

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

    m_sending = false;

    if (error) {
        cerr << m_name << ": send error: " << error.message() << endl;
        die();
        return;
    }
    else {
        cout << m_name << " sent " << bytes_transferred << endl;
        m_send_buffer.consume(bytes_transferred);
    }

    start_sending();
}

template<typename SocketType>
SocketType& Socket<SocketType>::socket() {
    return *m_socket;
}

template class Socket<asio::ip::tcp::socket>;
