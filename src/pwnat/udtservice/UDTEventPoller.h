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

#include <stdexcept>
#include <udt/udt.h>

/**
 * OO wrapper around UDT epoll functions
 */
class UDTEventPoller {
public:
    class Exception : public std::runtime_error {
    public:
        Exception(std::string what) : std::runtime_error(what) {}
    };

public:
    UDTEventPoller();
    ~UDTEventPoller();

    /*
     * Wait for events, times out after a while
     *
     * epoll_wait notes:
     * - a socket is returned in receive events <=> socket has data waiting for it in the receive buffer
     * - a socket is returned in send events <=> socket has room for new data in its send buffer (this is called over and over if you'd leave the socket registered to the write event with no data to send)
     *
     * receive_events: sockets that can receive
     * send_events: sockets that can send
     */
    void wait(std::set<UDTSOCKET>& receive_events, std::set<UDTSOCKET>& send_events);

    void add(const UDTSOCKET socket, int events);
    void remove(const UDTSOCKET socket);

private:
    void udt_throw(std::string method_name);

private:
    int m_poll_id;
};

