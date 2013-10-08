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
#include <udt/udt.h>

class UDTSocket;

/**
 * Polls for UDT events and dispatches io_service events
 */
class UDTService {
public:
    UDTService(boost::asio::io_service& io_service);

    void register_(UDTSocket& socket);

private: // TODO check all things are appropriately private/public
    void run();
    void process_requests();

private:
    boost::asio::io_service& m_io_service;
    boost::thread m_thread;
    int m_poll_id;
    std::map<UDTSOCKET, UDTSocket*> m_registrations;

    boost::mutex m_requests_lock; // registration requests, ...
    std::vector<UDTSocket*> m_pending_registrations;
};

