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
#include "TCPServer.h"

using namespace std;

class Client { // TODO rename pwnat::client::ProxyClient
public:
    Client() :
        m_tcp_server(m_io_service)
    {
    }

    void run() {
        cout << "running client" << endl;
        m_io_service.run();
    }

private:
    boost::asio::io_service m_io_service;
    TCPServer m_tcp_server;
};

