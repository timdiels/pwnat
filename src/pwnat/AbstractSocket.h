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
#include <pwnat/Disposable.h>
#include "SocketException.h"

/**
 * Abstract connection-oriented socket
 *
 * Sending and receiving can be done before the socket is connected, (though it
 * still has to be initialized first)
 *
 * Note on using Disposable pattern + shared_ptr (which the derived classes
 * use): Event notifications may still come in after the object is supposed to
 * be dead. Until then it has to stay alive and not respond to those
 * notifications
 */
class AbstractSocket : public Disposable {
public:
    /**
     * receive_buffer: whatever you don't consume will be included in a next call
     */
    typedef std::function<void(boost::asio::streambuf& receive_buffer)> ReceivedDataHandler;

    typedef std::function<void()> ConnectedHandler;
    typedef std::function<void()> DeathHandler;

public:
    AbstractSocket(bool connected, DeathHandler death_handler, std::string name);
    virtual ~AbstractSocket();

    bool dispose();

    /**
     * Must be called before any other methods
     */
    void init();

    /**
     * Bind instantly, then asynchronously connect
     *
     * source_port: if 0, a port is picked for you, otherwise given value is used
     *
     * REQUIRE(!connected())
     */
    virtual void connect(u_int16_t source_port, boost::asio::ip::address destination, u_int16_t destination_port) = 0;

    /**
     * Asynchronously sends data
     */
    void send(const char* data, size_t length);

    /**
     * Consumes buffer and asynchronously sends it
     */
    void send(boost::asio::streambuf& buffer);

    /**
     * Set handler that's called when socket received some data
     *
     * If data had already been received, handler is immediately called.
     */
    void on_received_data(ReceivedDataHandler);

    /**
     * Set handler that's called when socket is connected.
     *
     * If already connected, handler is immediately called.
     */
    void on_connected(ConnectedHandler);

    bool connected();

    /**
     * Using on_receive, from now on send whatever the given socket receives
     */
    virtual void receive_data_from(AbstractSocket& socket) = 0;

protected:
    /**
     * Asynchronously wait for messages
     *
     * May end up being called while already receiving.
     */
    virtual void start_receiving() = 0;

    /**
     * (Eventually) send everything in send buffer
     *
     * Called whenever something is added to the send buffer.
     *
     * May end up being called while already sending.
     */
    virtual void start_sending() = 0;

    /**
     * Notify listener of received data
     */
    void notify_received_data();

    /**
     * Notify listener that socket is connected
     */
    void notify_connected();

    /**
     * Dispose us and call DeathHandler
     */
    void die(const std::string& reason);

    void die(const std::string& prefix, const boost::system::error_code& error);

protected:
    boost::asio::streambuf m_receive_buffer;
    boost::asio::streambuf m_send_buffer;

    std::string m_name; // TODO might want to make private and provide a function to print error/info

private:
    bool m_connected;
    DeathHandler m_death_handler;

    ConnectedHandler m_connected_handler;
    ReceivedDataHandler m_received_data_handler;
};
