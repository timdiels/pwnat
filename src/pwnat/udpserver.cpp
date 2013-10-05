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
#include "checksum.h"

using namespace std;

const int udp_port_c = 22201;
const int udp_port_s = 22203;

void signal_handler(int sig);

template <typename chartype, typename iptype, typename tcptype>
class BasicPacket {
public:
    BasicPacket(chartype* data) : 
        m_data(data)
    {
        m_length = ntohs(ip_header()->ip_len);
    }

    iptype* ip_header() {
        return reinterpret_cast<iptype*>(m_data);
    }

    size_t ip_header_size() {
        return ip_header()->ip_hl * 4;
    }

    tcptype* tcp_header() {
        return reinterpret_cast<tcptype*>(m_data + ip_header_size());
    }

    chartype* data() {
        return m_data;
    }

    size_t length() {
        return m_length;
    }


private:
    chartype* m_data;
    size_t m_length;
};

typedef BasicPacket<char, ip, tcphdr> Packet;
typedef BasicPacket<const char, const ip, const tcphdr> ConstPacket;

class NetworkPipe {
public:
    /**
     * Push packet of given length onto pipe
     */
    virtual void push(ConstPacket&) = 0;
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
                    ConstPacket packet(boost::asio::buffer_cast<const char*>(buffer.data()));

                    if (buffer.size() >= packet.length()) {
                        cout << m_name << ": Received packet" << endl;
                        m_pipe.push(packet);
                        buffer.consume(packet.length());
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
     * Only let packets through that match given port
     */
    TCPPortFilter(NetworkPipe& pipe, unsigned short port) :
        m_pipe(pipe),
        m_port(port)
    {
    }

    void push(ConstPacket& packet) {
        if (ntohs(packet.tcp_header()->dest) == m_port) {
            m_pipe.push(packet);
        }
    }

private:
    NetworkPipe& m_pipe;
    unsigned short m_port;
};

/**
 * Rewrites source and dest ip address and tcp ports
 */
class Rewriter : public NetworkPipe {
public:
    Rewriter(NetworkPipe& pipe, boost::asio::ip::tcp::endpoint source, boost::asio::ip::tcp::endpoint destination) :
        m_pipe(pipe),
        m_source(source),
        m_destination(destination)
    {
    }

    void push(ConstPacket& packet) {
        char data[IP_MAXPACKET];
        memcpy(data, packet.data(), packet.length());
        Packet mut_packet(data);

        auto ip_hdr = mut_packet.ip_header();
        memcpy(&ip_hdr->ip_src, m_source.address().to_v4().to_bytes().data(), 4);
        memcpy(&ip_hdr->ip_dst, m_destination.address().to_v4().to_bytes().data(), 4);

        auto tcp_hdr = mut_packet.tcp_header();
        tcp_hdr->source = htons(m_source.port());
        tcp_hdr->dest = htons(m_destination.port());
        set_tcp_checksum(ip_hdr, tcp_hdr);

        ConstPacket new_packet(mut_packet.data());
        m_pipe.push(new_packet);
    }

private:
    NetworkPipe& m_pipe;
    boost::asio::ip::tcp::endpoint m_source;
    boost::asio::ip::tcp::endpoint m_destination;
};

/**
 * Sends pushed packets to connected ipv4 socket
 */
template <typename Socket>
class Sender : public NetworkPipe {
public:
    Sender(Socket& socket, string name) :
        m_socket(socket),
        m_name(name)
    {
    }

    void push(ConstPacket& packet) {
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
            abort();
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

/**
 * Sends pushed packets with disconnected socket to correct ip
 */
template <typename Socket>
class DisconnectedSender : public NetworkPipe { // TODO could fuse with Sender at the cost of having per Packet send/drop there too, which makes more sense slightly, conceptually
public:
    DisconnectedSender(Socket& socket, string name) :
        m_socket(socket),
        m_name(name)
    {
    }

    void push(ConstPacket& packet) {
        ostream ostr(&m_buffer);
        ostr.write(packet.data(), packet.length());
        if (m_buffer.size() == packet.length()) { // Don't spawn when a previous send is not yet complete
            boost::asio::spawn(m_socket.get_io_service(), boost::bind(&DisconnectedSender::send, this, _1));
        }
    }

    void send(boost::asio::yield_context yield) {
        while (m_buffer.size() > 0) {
            ConstPacket packet(boost::asio::buffer_cast<const char*>(m_buffer.data()));
            size_t length = packet.length() - packet.ip_header_size();
            m_buffer.consume(packet.ip_header_size());
            try {
                boost::asio::ip::udp::endpoint destination(boost::asio::ip::address::from_string("127.0.0.1"), 22);
                size_t bytes_transferred = m_socket.async_send_to(boost::asio::buffer(m_buffer.data(), length), destination, yield);
                cout << m_name << " sent " << bytes_transferred << "/" << length << endl;
            }
            catch (exception& e) {
                cerr << m_name << ": error: " << e.what() << endl;
            }

            // regardless of whether sending was a success, drop the packet
            m_buffer.consume(length);
        }
    }

private:
    boost::asio::streambuf m_buffer;
    Socket& m_socket;
    string m_name;
};


class Host {
public:
    Host(boost::asio::ip::tcp::endpoint source, boost::asio::ip::tcp::endpoint destination) :
        m_raw_socket(io_service, boost::asio::generic::raw_protocol(AF_INET, IPPROTO_TCP)),
        m_udp_socket(io_service),

        m_udp_sender(m_udp_socket, "udp sender"),
        m_filter(m_udp_sender, source.port()),
        m_raw_receiver(io_service, m_raw_socket, m_filter, "raw receiver"),

        m_raw_sender(m_raw_socket, "raw sender"),
        m_rewriter(m_raw_sender, source, destination),
        m_udp_receiver(io_service, m_udp_socket, m_rewriter, "udp receiver")
    {
        cout << source.port() << endl;
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
    DisconnectedSender<boost::asio::generic::raw_protocol::socket> m_raw_sender;
    Rewriter m_rewriter;
    Receiver<boost::asio::ip::udp::socket> m_udp_receiver;
};

class Client : public Host {
public:
    Client() : 
        Host(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 44401u),
             boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345u))
    {
    }

    void run() {
        m_udp_socket = boost::asio::ip::udp::socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), udp_port_c)),
        m_udp_socket.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), udp_port_s));

        cout << "running client" << endl;
        io_service.run();
    }
};

// TODO we can probably assume that upon each read exactly one packet is received, need to browse the web for that. We could switch to easier buffers too then, easier Packet formats too.
class Server : public Host {
public:
    Server() :
        Host(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 44403u),
             boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 22u))
    {
    }

    void run() {
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

