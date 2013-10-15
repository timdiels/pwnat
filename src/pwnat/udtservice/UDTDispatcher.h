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
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include "UDTEventPoller.h"

class UDTSocket;

/**
 * Dispatches one kind of UDT event to UDTSockets
 *
 * request_register and process_requests are thread safe. The other methods are
 * not, though they can safely be called while other threads are calling
 * request_register, process_requests.
 */
class UDTDispatcher {
public:
    typedef std::function<void()> Callback;

public:
    UDTDispatcher(boost::asio::io_service&, UDTEventPoller&, EPOLLOpt);

    void request_register(UDTSOCKET socket, Callback callback);

    /**
     * Process registration requests
     */
    void process_requests();

    /**
     * Register socket again with underlying poller/services
     */
    void reregister(UDTSOCKET);

    /**
     * Dispatch event to socket, and unregister socket
     *
     * Side-effect: if other dispatchers are using this socket, you must reregister the socket with them
     */
    void dispatch(UDTSOCKET);

    /**
     * Side-effect: if other dispatchers are using this socket, you must reregister the socket with them
     */
    void unregister(UDTSOCKET);

private:
    /**
     * Register socket to send events to
     */
    void register_(UDTSOCKET, Callback callback);

private:
    boost::asio::io_service& m_io_service;
    UDTEventPoller m_event_poller;
    const EPOLLOpt m_event;
    std::map<UDTSOCKET, boost::function<void()>> m_callbacks;

    boost::mutex m_requests_lock;
    std::vector<std::pair<UDTSOCKET, Callback>> m_requests;
};

