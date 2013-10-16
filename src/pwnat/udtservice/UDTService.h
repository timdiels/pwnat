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
#include "UDTEventPoller.h"
#include "UDTDispatcher.h"

class UDTSocket;

/**
 * Polls for UDT events and dispatches io_service events
 *
 * Note: should be used as a singleton
 */
class UDTService {
public:
    UDTService(boost::asio::io_service& io_service);

    /**
     * Notify UDTService that socket wants to receive data.
     *
     * UDTService will call the callback once (or twice) when there is data to receive with recv()
     */
    void request_receive(UDTSOCKET socket, UDTDispatcher::Callback callback);

    /**
     * Notify UDTService that socket wants to send
     *
     * UDTService will call the callback once (or twice) when there is room in buffer to send some data with send()
     */
    void request_send(UDTSOCKET socket, UDTDispatcher::Callback callback);

    /**
     * Unregister from all 
     */
    void request_unregister(UDTSOCKET socket);

    void stop();

private:
    void run() noexcept;
    void process_requests();

private:
    bool m_stopped;
    UDTEventPoller m_event_poller;
    UDTDispatcher m_receive_dispatcher;
    UDTDispatcher m_send_dispatcher;
    boost::thread m_thread;

    boost::mutex m_unregister_requests_lock;
    std::vector<UDTSOCKET> m_unregister_requests;
};

