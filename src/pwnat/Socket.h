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

#pragma once

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include "NetworkPipe.h"

/**
 * Wrapper around boost socket
 */
template <typename SocketType>
class Socket : public NetworkPipe {
public:
    /**
     * death_callback: called when the socket encounters a fatal error, disconnects, ...
     */
    Socket(SocketType& socket, NetworkPipe& pipe, boost::function<void()> death_callback);
    virtual ~Socket();

    void push(ConstPacket& packet);

    SocketType& socket();

private:
    void receive();
    void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);

    void send();
    void handle_send(const boost::system::error_code& error, size_t bytes_transferred);

private:
    SocketType& m_socket;
    std::string m_name;
    boost::function<void()> m_death_callback;

    // receive
    NetworkPipe& m_pipe;
    boost::array<char, 64 * 1024> m_receive_buffer;

    // send
    boost::asio::streambuf m_send_buffer;
};
typedef Socket<boost::asio::ip::tcp::socket> TCPSocket;

