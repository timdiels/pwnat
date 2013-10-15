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

#include "TCPServer.h"
#include "TCPClient.h"

#include <pwnat/namespaces.h>

TCPServer::TCPServer(asio::io_service& io_service) :
    m_udt_service(io_service),
    m_acceptor(io_service, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), 44401u)),
    m_next_flow_id(1u)
{
    accept();
}

void TCPServer::accept() {
    auto new_socket = new asio::ip::tcp::socket(m_acceptor.get_io_service());
    auto callback = bind(&TCPServer::handle_accept, this, asio::placeholders::error, new_socket);
    m_acceptor.async_accept(*new_socket, callback);
}

void TCPServer::handle_accept(const boost::system::error_code& error, asio::ip::tcp::socket* tcp_socket) {
    if (error) {
        cerr << "TCP Server: accept error: " << error.message() << endl;
    }
    else {
        cout << "New tcp client at port " << tcp_socket->remote_endpoint().port() << endl;
        try {
            new TCPClient(m_udt_service, tcp_socket, m_next_flow_id++);  // Note: ownership of socket transferred to TCPClient instance
        }
        catch (const exception& e) {
            cerr << "Failed to create client: " << e.what() << endl;
        }
        catch (...) {
            cerr << "Failed to create client: unknown error" << endl;
        }
    }

    accept();
}
