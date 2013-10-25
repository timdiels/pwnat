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
#include <memory>
#include "AbstractSocket.h"

class UDTService;

/**
 * Convenient rendezvous UDT socket for sending/receiving
 *
 */
class UDTSocket : public AbstractSocket, public std::enable_shared_from_this<UDTSocket> {
public:
    /**
     * Construct a socket that has yet to connect
     */
    UDTSocket(UDTService&, DeathHandler);
    ~UDTSocket();

    void connect(u_int16_t source_port, boost::asio::ip::address destination, u_int16_t destination_port);
    void receive_data_from(AbstractSocket& socket);
    bool dispose();

    /*
     * Get currently bound port.
     *
     * Must call connect first, though needn't be connected yet
     */
    u_int16_t local_port();

protected:
    void start_receiving();
    void start_sending();

private:
    void handle_receive();
    void handle_send();

private:
    UDTService& m_udt_service;
    UDTSOCKET m_socket;
};

