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
#include <memory>
#include "NetworkPipe.h"
#include "Disposable.h"

class UDTService;

// TODO once TCPSocket is shared too, you'll have to unset the pipe upon dispose
/**
 * Convenient rendezvous UDT socket for sending/receiving
 *
 * Note on using Disposable pattern + shared_ptr: Event notifications may still
 * come in after the object is supposed to be dead. Until then it has to stay
 * alive and not respond to those notifications
 */
class UDTSocket : public NetworkPipe, public Disposable, public std::enable_shared_from_this<UDTSocket> {
public:
    UDTSocket(UDTService& udt_service, u_int16_t source_port, u_int16_t destination_port, boost::asio::ip::address_v4 destination, boost::function<void()> death_callback);

    /**
     * Closes the socket
     */
    ~UDTSocket();

    void init();
    void dispose();
    void pipe(std::shared_ptr<NetworkPipe> pipe);

    /**
     * Call handler once once connection is established
     */
    void add_connection_listener(boost::function<void()> handler);

    void push(ConstPacket& packet);
    UDTSOCKET socket();

private:
    void receive();
    void send();

    void request_receive();
    void request_send();

private:
    boost::function<void()> m_death_callback;
    UDTService& m_udt_service;
    UDTSOCKET m_socket;
    std::string m_name;

    bool m_connected;
    std::vector<boost::function<void()>> m_connection_listeners;

    // receive
    std::shared_ptr<NetworkPipe> m_pipe;

    // send
    boost::asio::streambuf m_buffer;
};

