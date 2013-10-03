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

#include <vector>
#include <boost/asio.hpp>

/**
 * Tunnel over UDP in a reliable, ordered delivery  that sends out a keep alive when necessary (for hole punching)
 */
class TCPClient : public boost::enable_shared_from_this<tcp_connection> {
public:
    typedef boost::shared_ptr<TCPClient> pointer;

    static pointer create(boost::asio::io_service& io_service) {
        return pointer(new TCPClient(io_service));
    }

    tcp::socket& socket() {
        return socket_;
    }

    void start() {
        udp_socket->start();
        // TODO wait for udp socket to be alive (icmp timeout, hole punching)

        auto callback = boost::bind(&TCPClient::handle_receive, shared_from_this();
        tcp_socket.async_receive(bufs), boost::asio::buffer(message_), callback));
    }

private:
    TCPClient(boost::asio::io_service& io_service) : socket_(io_service) {}
    void handle_write() {}

    boost::asio::ip::tcp::socket tcp_socket_;
    boost::asio::streambuf buffer_;
};

