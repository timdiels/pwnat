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
    m_epoll_events({UDT_EPOLL_IN, UDT_EPOLL_OUT}),
    m_io_service(io_service),
    m_thread(boost::bind(&UDTService::run, this))
{
    for (auto event : m_epoll_events) {
        m_sockets[event] = new map<UDTSOCKET, UDTSocket*>;
    }
}

UDTService::~UDTService() {
    UDT::epoll_release(m_poll_id);
}

void UDTService::request_receive(UDTSocket& socket) {
    boost::lock_guard<boost::mutex> guard(m_requests_lock);
    m_requests.push_back(make_pair(&socket, UDT_EPOLL_IN));
}

void UDTService::request_send(UDTSocket& socket) {
    boost::lock_guard<boost::mutex> guard(m_requests_lock);
    m_requests.push_back(make_pair(&socket, UDT_EPOLL_OUT));
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
    const int64_t timeout_ms = 1000;

    /*
     * epoll_wait notes:
     * - a socket is returned in receive events <=> socket has data waiting for it in the receive buffer
     * - a socket is returned in send events <=> socket has room for new data in its send buffer (this is called over and over if you'd leave the socket registered to the write event with no data to send)
     */
    while (true) {
        cout << "waiting" << endl;
        if (UDT::epoll_wait(m_poll_id, &receive_events, &send_events, timeout_ms) < 0) {
            cerr << "epoll_wait failed" << endl;
            abort();
        }

        // Dispatch events to sockets that can read
        for (auto socket_handle : receive_events) {
            auto socket = m_sockets.at(UDT_EPOLL_IN)->at(socket_handle);
            cout << "dispatch receive" << endl;

            // unregister so we don't notify a thousand times
            UDT::epoll_remove_usock(socket->socket(), UDT_EPOLL_IN);

            // dispatch
            m_io_service.dispatch(boost::bind(&UDTSocket::receive, socket));
        }

        // Dispatch events to sockets that can write
        for (auto socket_handle : send_events) {
            auto socket = m_sockets.at(UDT_EPOLL_OUT)->at(socket_handle);
            cout << "dispatch send" << endl;

            // unregister so we don't notify a thousand times
            UDT::epoll_remove_usock(socket->socket(), UDT_EPOLL_OUT);

            // dispatch
            m_io_service.dispatch(boost::bind(&UDTSocket::send, socket));
        }

        // Process pending registrations
        process_requests();
    }
}

void UDTService::process_requests() {
    boost::lock_guard<boost::mutex> guard(m_requests_lock);
    for (auto request : m_requests) {
        cout << "add usock: " << request.second << endl;
        
        UDTSocket* socket = request.first;
        epoll_add_usock(socket->socket(), request.second);
        (*m_sockets.at(request.second))[socket->socket()] = socket;
    }
    m_requests.clear();
}

void UDTService::epoll_add_usock(const UDTSOCKET socket, int events) {
    if (UDT::epoll_add_usock(m_poll_id, socket, &events) < 0) {
        cerr << "epoll_add_usock failed" << endl;
        abort();
    }
}

void UDTService::epoll_remove_usock(const UDTSOCKET socket, int events) {
    // remove from all event registrations
    if (UDT::epoll_remove_usock(m_poll_id, socket) < 0) {
        cerr << "epoll_remove_usock failed" << endl;
        abort();
    }

    // reregister for events it should still be registered for
    int left_over_events = 0;  // events that it should still be registered for
    for (auto event : m_epoll_events) {
        auto& sockets = m_sockets[event];
        if (events & event) {
            sockets->erase(socket);
        }
        else if (sockets->find(socket) != sockets->end()) {
            left_over_events |= event;
        }
    }

    if (left_over_events) {
        epoll_add_usock(socket, left_over_events);
    }
}
