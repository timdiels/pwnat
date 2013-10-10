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

template<typename SocketType>
Socket<SocketType>::Socket(SocketType& socket, shared_ptr<NetworkPipe> pipe, boost::function<void()> death_callback) : 
    m_socket(socket),
    m_name("tcp socket"),
    m_death_callback(death_callback),
    m_pipe(pipe)
{
    receive();
}
// TODO name + tcp client port, or no names, or name defaults to: proto sender/receiver src->dst port (let's do that latter)

template<typename SocketType>
Socket<SocketType>::~Socket() {
}

template<typename SocketType>
SocketType& Socket<SocketType>::socket() {
    return m_socket;
}

template<typename SocketType>
void Socket<SocketType>::receive() {
    auto callback = boost::bind(&Socket::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
    m_socket.async_receive(boost::asio::buffer(m_receive_buffer), callback);
}

template<typename SocketType>
void Socket<SocketType>::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
    if (error) {
        cerr << m_name << ": receive error: " << error.message() << endl;
        m_death_callback();
        return;
    }
    else {
        cout << m_name << " received " << bytes_transferred << endl;
        ConstPacket packet(m_receive_buffer.data(), bytes_transferred); // TODO can't we just hand it to a string object or such?
        m_pipe->push(packet);
    }

    receive();
}

template<typename SocketType>
void Socket<SocketType>::push(ConstPacket& packet) {
    ostream ostr(&m_send_buffer);
    ostr.write(packet.data(), packet.length());
    send();
}

template<typename SocketType>
void Socket<SocketType>::send() {
    if (m_send_buffer.size() > 0) {
        auto callback = boost::bind(&Socket::handle_send, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        m_socket.async_send(m_send_buffer.data(), callback);
    }
}

template<typename SocketType>
void Socket<SocketType>::handle_send(const boost::system::error_code& error, size_t bytes_transferred) {
    if (error) {
        cerr << m_name << ": send error: " << error.message() << endl;
        m_death_callback();
        return;
    }
    else {
        cout << m_name << " sent " << bytes_transferred << endl;
        m_send_buffer.consume(bytes_transferred);
        send();
    }
}

template class Socket<boost::asio::ip::tcp::socket>;
