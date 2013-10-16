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

#include <pwnat/namespaces.h>

UDTService::UDTService(asio::io_service& io_service) :
    m_stopped(false),
    m_receive_dispatcher(io_service, m_event_poller, UDT_EPOLL_IN),
    m_send_dispatcher(io_service, m_event_poller, UDT_EPOLL_OUT),
    m_thread(bind(&UDTService::run, this))
{
}

void UDTService::request_receive(UDTSOCKET socket, UDTDispatcher::Callback callback) {
    m_receive_dispatcher.request_register(socket, callback);
}

void UDTService::request_send(UDTSOCKET socket, UDTDispatcher::Callback callback) {
    m_send_dispatcher.request_register(socket, callback);
}

void UDTService::request_unregister(UDTSOCKET socket) {
    boost::lock_guard<boost::mutex> guard(m_unregister_requests_lock);
    m_unregister_requests.push_back(socket);
}

void UDTService::run() noexcept {
    try {
        cout << "UDT service thread started" << endl;

        set<UDTSOCKET> receive_events; // sockets that can receive
        set<UDTSOCKET> send_events; // sockets that can send

        /*
         * epoll_wait notes:
         * - a socket is returned in receive events <=> socket has data waiting for it in the receive buffer
         * - a socket is returned in send events <=> socket has room for new data in its send buffer (this is called over and over if you'd leave the socket registered to the write event with no data to send)
         */
        while (!m_stopped) {
            try {
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
            }
            catch (const UDTEventPoller::Exception& e) {
                cerr << "Warning: " << e.what() << endl;
            }

            // Process pending requests
            process_requests();
        }

        cout << "UDT service thread stopped" << endl;
    }
    catch (const exception& e) {
        cerr << "UDT service thread crashed: " << e.what() << endl;
    }
    catch (...) {
        cerr << "UDT service thread crashed: unknown error" << endl;
    }

    abort();
}

void UDTService::process_requests() {
    m_receive_dispatcher.process_requests();
    m_send_dispatcher.process_requests();

    boost::lock_guard<boost::mutex> guard(m_unregister_requests_lock);
    for (auto socket : m_unregister_requests) {
        m_receive_dispatcher.unregister(socket);
        m_send_dispatcher.unregister(socket);
    }
    m_unregister_requests.clear();
}

void UDTService::stop() {
    m_stopped = true;
}

