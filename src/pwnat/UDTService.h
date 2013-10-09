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
#include <boost/thread.hpp>
#include <boost/array.hpp>
#include "UDTEventPoller.h"

class UDTSocket;

/**
 * Polls for UDT events and dispatches io_service events
 */
class UDTService {
public:
    UDTService(boost::asio::io_service& io_service);
    ~UDTService();

    /*
     * Notify UDTService that socket wants to receive data.
     *
     * UDTService will notify UDTSocket once (or twice) when there is data to receive with recv()
     */
    void request_receive(UDTSocket& socket);

    /*
     * Notify UDTService that socket wants to send
     *
     * UDTService will notify UDTSocket once (or twice) when there is room in buffer to send some data with send()
     */
    void request_send(UDTSocket& socket);

private: // TODO check all things are appropriately private/public
    void run();
    void process_requests();
    void epoll_remove_usock(const UDTSOCKET socket, int events);  // unregister events from socket

private:
    const boost::array<EPOLLOpt, 2> m_epoll_events;
    boost::asio::io_service& m_io_service;
    boost::thread m_thread;
    UDTEventPoller m_event_poller;
    std::map<EPOLLOpt, std::map<UDTSOCKET, UDTSocket*>*> m_sockets;

    boost::mutex m_requests_lock;
    std::vector<std::pair<UDTSocket*, EPOLLOpt>> m_requests;
};

