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

#include "UDTEventPoller.h"
#include <iostream>

using namespace std;

UDTEventPoller::UDTEventPoller()
{
    m_poll_id = UDT::epoll_create();
    if (m_poll_id < 0) {
        cerr << "epoll_create failed" << endl;
        abort();
    }
}

UDTEventPoller::~UDTEventPoller() {
    UDT::epoll_release(m_poll_id);
}

void UDTEventPoller::wait(set<UDTSOCKET>& receive_events, set<UDTSOCKET>& send_events) {
    const int64_t timeout_ms = 100;
    if (UDT::epoll_wait(m_poll_id, &receive_events, &send_events, timeout_ms) < 0) {
        cerr << "Warning: epoll_wait failed" << endl;
    }
}

void UDTEventPoller::add(const UDTSOCKET socket, int events) {
    if (UDT::epoll_add_usock(m_poll_id, socket, &events) < 0) {
        cerr << "epoll_add_usock failed" << endl;
        abort();
    }
}

void UDTEventPoller::remove(const UDTSOCKET socket) {
    if (UDT::epoll_remove_usock(m_poll_id, socket) < 0) {
        cerr << "epoll_remove_usock failed" << endl;
        abort();
    }
}

