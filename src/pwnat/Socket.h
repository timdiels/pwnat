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

#include <memory>
#include "AbstractSocket.h"

/**
 * Wrapper around boost socket
 */
template <typename SocketType>
class Socket : public AbstractSocket, public std::enable_shared_from_this<Socket<SocketType>> {
public:
    /**
     * Construct a socket with an already connected socket
     */
    Socket(std::shared_ptr<SocketType> socket, DeathHandler);

    /**
     * Construct a socket that has yet to connect
     */
    Socket(boost::asio::io_service&, DeathHandler);

    void connect(u_int16_t source_port, boost::asio::ip::address destination, u_int16_t destination_port);
    void receive_data_from(AbstractSocket& socket);

protected:
    void start_receiving();
    void start_sending();

private:
    void handle_connected(boost::system::error_code error);
    void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_send(const boost::system::error_code& error, size_t bytes_transferred);

private:
    std::shared_ptr<SocketType> m_socket;
    bool m_receiving;
    bool m_sending;
};
typedef Socket<boost::asio::ip::tcp::socket> TCPSocket;

