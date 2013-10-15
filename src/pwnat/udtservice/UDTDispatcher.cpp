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

#include "UDTDispatcher.h"
#include <iostream>

#include <pwnat/namespaces.h>

UDTDispatcher::UDTDispatcher(asio::io_service& io_service, UDTEventPoller& event_poller, EPOLLOpt event) :
    m_io_service(io_service),
    m_event_poller(event_poller),
    m_event(event)
{
}

void UDTDispatcher::request_register(UDTSOCKET socket, Callback callback) {
    boost::lock_guard<boost::mutex> guard(m_requests_lock);
    m_requests.push_back(make_pair(socket, callback));
}

void UDTDispatcher::register_(UDTSOCKET socket, Callback callback) {
    cout << "register: " << m_event << endl;
    
    try {
        m_event_poller.add(socket, m_event);
        m_callbacks[socket] = callback;
    }
    catch (UDTEventPoller::Exception& e) {
        cerr << "Warning: " << e.what() << endl;
    }
}

void UDTDispatcher::unregister(UDTSOCKET socket) {
    try {
        m_event_poller.remove(socket);
    }
    catch (UDTEventPoller::Exception& e) {
        cerr << "Warning: " << e.what() << endl;
    }
    m_callbacks.erase(socket);
}

void UDTDispatcher::reregister(UDTSOCKET socket) {
    if (m_callbacks.find(socket) != m_callbacks.end()) {
        try {
            m_event_poller.add(socket, m_event);
        }
        catch (UDTEventPoller::Exception& e) {
            cerr << "Warning: " << e.what() << endl;
        }
    }
}

void UDTDispatcher::dispatch(UDTSOCKET socket) {
    cout << "dispatch " << m_event << endl;
    m_io_service.dispatch(m_callbacks.at(socket));
    unregister(socket);
}

void UDTDispatcher::process_requests() {
    boost::lock_guard<boost::mutex> guard(m_requests_lock);
    for (auto request : m_requests) {
        register_(request.first, request.second);
    }
    m_requests.clear();
}

