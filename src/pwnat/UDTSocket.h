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

#include <udt/udt.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include "NetworkPipe.h"

class UDTService;

/**
 * Convenient rendezvous UDT socket for sending/receiving
 */
class UDTSocket : public NetworkPipe {
public:
    UDTSocket(UDTService& udt_service, u_int16_t source_port, u_int16_t destination_port, boost::asio::ip::address_v4 destination);

    void pipe(NetworkPipe& pipe);

    /**
     * Call handler once once connection is established
     */
    void add_connection_listener(boost::function<void()> handler);

    void push(ConstPacket& packet);

    /**
     * REQUIRE(internal socket is connected)
     */
    void send(); // TODO private

    void receive(); // TODO private
    UDTSOCKET socket();

private:
    UDTService& m_udt_service;
    UDTSOCKET m_socket;
    std::string m_name;

    bool m_connected;
    std::vector<boost::function<void()>> m_connection_listeners;

    // receive
    NetworkPipe* m_pipe;

    // send
    boost::asio::streambuf m_buffer;
};

