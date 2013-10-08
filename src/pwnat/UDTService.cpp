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

#include "UDTService.h"
#include "UDTSocket.h"

using namespace std;

UDTService::UDTService(boost::asio::io_service& io_service) :
    m_io_service(io_service)
    //m_thread(boost::bind(&UDTService::run, this)) TODO
{
}

void UDTService::register_(UDTSocket& socket) {
    boost::lock_guard<boost::mutex> guard(m_requests_lock);
    m_pending_registrations.push_back(&socket);
}

void UDTService::run() {
    cout << "UDT service thread started" << endl;

    m_poll_id = UDT::epoll_create();
    if (m_poll_id < 0) {
        cerr << "epoll_create failed" << endl;
        abort();
    }

    set<UDTSOCKET> receive_events; // sockets that can receive
    set<UDTSOCKET> send_events; // sockets that can send
    const int64_t timeout_ms = 500;

    while (true) {
        if (UDT::epoll_wait(m_poll_id, &receive_events, &send_events, timeout_ms) < 0) {
            cerr << "epoll_wait failed" << endl;
            abort();
        }

        // Dispatch events to sockets that can read
        for (auto socket_handle : receive_events) {
            auto socket = m_registrations.at(socket_handle);
            cout << "dispatch receive" << endl;
            m_io_service.dispatch(boost::bind(&UDTSocket::receive, socket));
        }

        // Dispatch events to sockets that can write
        for (auto socket_handle : send_events) {
            auto socket = m_registrations.at(socket_handle);
            cout << "dispatch send" << endl;
            m_io_service.dispatch(boost::bind(&UDTSocket::send, socket));
        }

        // Process pending registrations
        process_requests();
    }
}

void UDTService::process_requests() {
    boost::lock_guard<boost::mutex> guard(m_requests_lock);
    for (auto registration : m_pending_registrations) {
        cout << "register" << endl;
        
        if (UDT::epoll_add_usock(m_poll_id, registration->socket()) < 0) {
            cerr << "epoll_add_usock failed" << endl;
            abort();
        }
        m_registrations[registration->socket()] = registration;
    }
    m_pending_registrations.clear();
}

