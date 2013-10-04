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

#include <signal.h>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include "socket.h"

using namespace std;

const int tcp_port_c = 44401;
const int tcp_port_s = 44403;
const int udp_port_c = 22201;
const int udp_port_s = 22203;

void signal_handler(int sig);

class Packet {
public:
    Packet(char* data, size_t length) : m_data(data), m_length(length)
    {
    }

    ip* ip_header() {
        return reinterpret_cast<ip*>(m_data);
    }

    tcphdr* tcp_header() {
        return reinterpret_cast<tcphdr*>(m_data + ip_header()->ip_hl * 4);
    }

    char* data() {
        return m_data;
    }

    size_t length() {
        return m_length;
    }


private:
    char* m_data;
    size_t m_length;
};

class NetworkPipe {
public:
    /**
     * Push packet of given length onto pipe
     */
    virtual void push(Packet&) = 0;
};

/**
 * Receives from ipv4 socket and pushes to a NetworkPipe
 */
template <typename Socket>
class Receiver {
public:
    Receiver(boost::asio::io_service& io_service, Socket& socket, NetworkPipe& pipe, string name) : 
        m_socket(socket),
        m_pipe(pipe),
        m_name(name)
    {
        boost::asio::spawn(io_service, boost::bind(&Receiver::receive, this, _1));
    }

    void receive(boost::asio::yield_context yield) {
        boost::asio::streambuf buffer;
        while (true) {
            try {
                size_t bytes_received = m_socket.async_receive(buffer.prepare(IP_MAXPACKET), yield);
                buffer.commit(bytes_received);

                if (buffer.size() > 4) {
                    auto data = boost::asio::buffer_cast<const char*>(buffer.data());
                    auto ip_hdr = reinterpret_cast<const ip*>(data);

                    auto packet_length = ntohs(ip_hdr->ip_len);
                    if (buffer.size() >= packet_length) {
                        cout << m_name << ": Received packet" << endl;
                        char packet_data[IP_MAXPACKET];
                        Packet packet(packet_data, packet_length);
                        memcpy(packet.data(), data, packet_length);
                        m_pipe.push(packet);

                        buffer.consume(packet_length);
                    }
                }
            }
            catch (std::exception& e) {
                cerr << m_name << ": error: " << e.what() << endl;
                abort();
            }
        }
    }

private:
    Socket& m_socket;
    NetworkPipe& m_pipe;
    string m_name;
};

/**
 * Filter packets by port
 */
class TCPPortFilter : public NetworkPipe {
public:

    /**
     * Only let packets through that match port
     */
    TCPPortFilter(NetworkPipe& pipe) :
        m_pipe(pipe)
    {
    }

    void push(Packet& packet) {
        if (ntohs(packet.tcp_header()->dest) == tcp_port_c) {
            m_pipe.push(packet);
        }
    }

private:
    NetworkPipe& m_pipe;
};

/**
 * Sends pushed packets to ipv4 socket
 */
template <typename Socket>
class Sender : public NetworkPipe {
public:
    Sender(Socket& socket, string name) :
        m_socket(socket),
        m_name(name)
    {
    }

    void push(Packet& packet) {
        ostream ostr(&m_buffer);
        ostr.write(packet.data(), packet.length());
        send();
    }

    void send() {
        if (m_buffer.size() > 0) {
            auto callback = boost::bind(&Sender::handle_send, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
            m_socket.async_send(m_buffer.data(), callback);
        }
    }

    void handle_send(const boost::system::error_code& error, size_t bytes_transferred) {
        if (error) {
            cerr << m_name << ": error: " << error.message() << endl;
        }
        else {
            cout << m_name << " sent " << bytes_transferred << endl;
            m_buffer.consume(bytes_transferred);
            send();
        }
    }

private:
    boost::asio::streambuf m_buffer;
    Socket& m_socket;
    string m_name;
};



class Client {
public:
    Client() :
        m_raw_socket(io_service, boost::asio::generic::raw_protocol(AF_INET, IPPROTO_TCP)),
        m_udp_socket(io_service),

        m_udp_sender(m_udp_socket, "udp sender"),
        m_filter(m_udp_sender),
        m_raw_receiver(io_service, m_raw_socket, m_filter, "raw receiver"),

        m_raw_sender(m_raw_socket, "raw sender"),
        m_udp_receiver(io_service, m_udp_socket, m_raw_sender, "udp receiver")
    {
    }

    void run() {
        m_udp_socket = boost::asio::ip::udp::socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), udp_port_c)),
        m_udp_socket.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), udp_port_s));

        cout << "running client" << endl;
        io_service.run();
    }

protected:
    boost::asio::io_service io_service;
    boost::asio::generic::raw_protocol::socket m_raw_socket;
    boost::asio::ip::udp::socket m_udp_socket;

    // raw -> udp
    Sender<boost::asio::ip::udp::socket> m_udp_sender;
    TCPPortFilter m_filter;
    Receiver<boost::asio::generic::raw_protocol::socket> m_raw_receiver;

    // udp -> raw
    Sender<boost::asio::generic::raw_protocol::socket> m_raw_sender;
    Receiver<boost::asio::ip::udp::socket> m_udp_receiver;
};



class Server : public Client {
public:
    Server() :
        Client()
    {
    }

    void run() {
        // TODO needs filter to be of tcp_port_s, will also have different rewrite system than Client
        m_udp_socket = boost::asio::ip::udp::socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), udp_port_s));
        m_udp_socket.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), udp_port_c));

        cout << "running server" << endl;
        io_service.run();
    }
};


/*
 * UDP Tunnel server main(). Handles program arguments, initializes everything,
 * and runs the main loop.
 */
int udpserver(int argc, char *argv[])
{
    try {
        signal(SIGINT, &signal_handler);

        // prototype code
        if (argc == 0) {
            // pretend to be client
            Client client;
            client.run();
        }
        else {
            // pretend to be server
            Server server;
            server.run();
        }

        /*while(running) {
        }*/
    } catch (exception& e) {
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}

void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGINT:
            abort();
    }
}

