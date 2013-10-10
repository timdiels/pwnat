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
#include <pwnat/udtservice/UDTService.h>

class TCPServer {
public:
    TCPServer(boost::asio::io_service& io_service);

    void accept();
    void handle_accept(const boost::system::error_code& error, boost::asio::ip::tcp::socket* tcp_socket);

private:
    UDTService m_udt_service;
    boost::asio::ip::tcp::acceptor m_acceptor;
};

