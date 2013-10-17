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

#include <pwnat/Application.h>

class TCPServer : public Application {
public:
    TCPServer(ProgramArgs&);

private:
    void accept();
    void handle_accept(const boost::system::error_code& error, boost::asio::ip::tcp::socket* tcp_socket);

private:
    boost::asio::ip::tcp::acceptor m_acceptor;
    u_int16_t m_next_flow_id;
};

