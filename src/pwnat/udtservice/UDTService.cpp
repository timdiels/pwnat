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
#include <cassert>
#include <pwnat/UDTSocket.h>

using namespace std;

UDTService::UDTService(boost::asio::io_service& io_service) :
    m_receive_dispatcher(io_service, m_event_poller, UDT_EPOLL_IN),
    m_send_dispatcher(io_service, m_event_poller, UDT_EPOLL_OUT),
    m_thread(boost::bind(&UDTService::run, this))
{
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

    set<UDTSOCKET> receive_events; // sockets that can receive
    set<UDTSOCKET> send_events; // sockets that can send

    /*
     * epoll_wait notes:
     * - a socket is returned in receive events <=> socket has data waiting for it in the receive buffer
     * - a socket is returned in send events <=> socket has room for new data in its send buffer (this is called over and over if you'd leave the socket registered to the write event with no data to send)
     */
    while (true) {
        cout << ".";
        m_event_poller.wait(receive_events, send_events);

        // Dispatch events to sockets that can read
        for (auto socket_handle : receive_events) {
            m_receive_dispatcher.dispatch(socket_handle);
            m_send_dispatcher.reregister(socket_handle);
        }

        // Dispatch events to sockets that can write
        for (auto socket_handle : send_events) {
            m_send_dispatcher.dispatch(socket_handle);
            m_receive_dispatcher.reregister(socket_handle);
        }

        // Process pending registrations
        process_requests();
    }
}

void UDTService::process_requests() {
    boost::lock_guard<boost::mutex> guard(m_requests_lock);
    for (auto request : m_requests) {
        UDTSocket* socket = request.first;

        if (request.second == UDT_EPOLL_IN) {
            m_receive_dispatcher.register_(socket->socket(), boost::bind(&UDTSocket::receive, socket));
        }
        else {
            assert(request.second == UDT_EPOLL_OUT);
            m_send_dispatcher.register_(socket->socket(), boost::bind(&UDTSocket::send, socket));
        }
    }
    m_requests.clear();
}

