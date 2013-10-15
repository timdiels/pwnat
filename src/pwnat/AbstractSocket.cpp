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

#include "AbstractSocket.h"
#include <pwnat/namespaces.h>

AbstractSocket::AbstractSocket(bool connected, DeathHandler death_handler, string name) :
    m_name(name),
    m_connected(connected),
    m_death_handler(death_handler),
    m_connected_handler([](){}),
    m_received_data_handler([](asio::streambuf&){})
{
}

AbstractSocket::~AbstractSocket() {
    dispose();
    cout << m_name << ": Deallocated" << endl;
}

bool AbstractSocket::dispose() {
    if (Disposable::dispose()) {
        // unset all handlers (there might be shared_ptr in them)
        m_death_handler = DeathHandler();
        m_connected_handler = ConnectedHandler();
        m_received_data_handler = ReceivedDataHandler();
        return true;
    }
    else {
        return false;
    }
}

void AbstractSocket::init() {
    if (disposed()) return;
    if (m_connected) {
        m_connected = false;
        notify_connected();
    }
}

void AbstractSocket::send(const char* data, size_t length) {
    if (disposed()) return;

    ostream ostr(&m_send_buffer);
    ostr.write(data, length);
    if (connected()) {
        start_sending();
    }
}

void AbstractSocket::send(asio::streambuf& buffer) {
    if (disposed()) return;

    ostream ostr(&m_send_buffer);
    ostr.write(asio::buffer_cast<const char*>(buffer.data()), buffer.size());
    buffer.consume(buffer.size());

    assert(buffer.size() == 0); // TODO rm
    assert(m_send_buffer.size() > 0);
    if (connected()) {
        start_sending();
    }
}

void AbstractSocket::on_received_data(ReceivedDataHandler handler) {
    if (disposed()) return;
    m_received_data_handler = handler;
    if (m_receive_buffer.size()) {
        notify_received_data();
    }
}

void AbstractSocket::on_connected(ConnectedHandler handler) {
    if (disposed()) return;

    if (m_connected) {
        handler();
    }
    else {
        m_connected_handler = handler;
    }
}

bool AbstractSocket::connected() {
    return m_connected;
}

void AbstractSocket::notify_received_data() {
    m_received_data_handler(m_receive_buffer);
}

void AbstractSocket::notify_connected() {
    if (!m_connected) {
        m_connected = true;
        m_connected_handler();
        start_receiving();
        start_sending();
    }
}

void AbstractSocket::die() {
    m_death_handler();
    dispose();
    // TODO throw exception, seriously we need to, and handle it where appropriate (if anywhere)
}

