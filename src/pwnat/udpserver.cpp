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
#include <udt/udt.h>
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

// TODO you can bind your raw socket to local address instead of doing TCPPortFilter, apparently
/**
 * Receives from ipv4 socket and pushes to a NetworkPipe
 */
template <typename Socket>
class Receiver {
public:
    Receiver(Socket& socket, NetworkPipe& pipe, string name) : 
        m_socket(socket),
        m_pipe(pipe),
        m_name(name)
    {
        boost::asio::spawn(socket.get_io_service(), boost::bind(&Receiver::receive, this, _1));
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
                //abort();
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
 * Sends pushed packets to connected UDT socket
 */
class UDTSender : public NetworkPipe {
public:
    UDTSender(UDTSOCKET socket, string name) :
        m_new(new boost::asio::streambuf),
        m_sending(new boost::asio::streambuf),
        m_socket(socket),
        m_name(name)
    {
    }

    void push(ConstPacket& packet) {
        // TODO lock
        ostream ostr(m_new);
        ostr.write(packet.data(), packet.length()); // TODO might cause reallocation, which invalidates horrifically elsewhere?
        send(); // TODO do this asynchronously, or epoll. In any case epoll, perhaps one thread epoll eventually
    }

    void send() {
        // Note: runs in a thread different from that of io_service handlers
        swap(m_sending, m_new); // TODO lock

        while (m_sending->size() > 0) {
            int bytes_transferred = UDT::send(m_socket, boost::asio::buffer_cast<const char*>(m_sending->data()), m_sending->size(), 0);
            if (bytes_transferred == UDT::ERROR) {
                cerr << m_name << ": error: " << UDT::getlasterror().getErrorMessage() << endl;
                abort();
            }
            else {
                cout << m_name << " sent " << bytes_transferred << endl;
                m_sending->consume(bytes_transferred);
            }
        }
    }

private:
    boost::asio::streambuf* m_new;
    boost::asio::streambuf* m_sending;
    UDTSOCKET m_socket;
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

class TCPToUDPMapping {
public:
    /**
     * tcp_server: endpoint of the tcp server
     * tcp_client: endpoint of the client that connected to our tcp server
     */
    TCPToUDPMapping(boost::asio::io_service& io_service, DisconnectedSender<boost::asio::generic::raw_protocol::socket>& raw_sender, boost::asio::ip::tcp::endpoint tcp_server, boost::asio::ip::tcp::endpoint tcp_client) :
        m_udp_socket(UDT::socket(AF_INET, SOCK_STREAM, 0)),

        m_udp_sender(m_udp_socket, "udt sender"),
        // TODO next pipe last, name reeeally last
        // TODO name + tcp client port, or no names, or name defaults to: udp sender src->dst port (let's do that latter)

        m_rewriter(raw_sender, tcp_server, tcp_client)
        //m_udp_receiver(m_udp_socket, m_rewriter, "udp receiver")
    {
        assert(m_udp_socket != UDT::INVALID_SOCK); // todo exception throw

        sockaddr_in localhost;
        localhost.sin_family = AF_INET;
        localhost.sin_addr.s_addr = inet_addr("127.0.0.1");
        memset(&localhost.sin_zero, 0, 8);

        localhost.sin_port = htons(udp_port_c);
        if (UDT::ERROR == UDT::bind(m_udp_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
            cerr << "client bind error: " << UDT::getlasterror().getErrorMessage() << endl;
            abort();
        }

        localhost.sin_port = htons(udp_port_s);
        if (UDT::ERROR == UDT::connect(m_udp_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
            cerr << "client connect error: " << UDT::getlasterror().getErrorMessage() << endl;
            abort();
        }
    }

    NetworkPipe& udp_sender() {
        return m_udp_sender;
    }

private:
    UDTSOCKET m_udp_socket; // TODO rename m_udt_socket

    UDTSender m_udp_sender;

    Rewriter m_rewriter;
    //Receiver<boost::asio::ip::udp::socket> m_udp_receiver;
};

class TCPToUDPRouter : public NetworkPipe {
public:
    TCPToUDPRouter(boost::asio::generic::raw_protocol::socket& raw_socket, boost::asio::ip::tcp::endpoint source) :
        m_io_service(raw_socket.get_io_service()),
        m_tcp_server(source),
        m_raw_sender(raw_socket, "raw sender")
    {
    }

    void push(ConstPacket& packet) {
        u_int16_t client_port = htons(packet.tcp_header()->source);
        if (m_mappings.find(client_port) == m_mappings.end()) {
            cout << "New tcp client at port " << client_port << endl;
            boost::asio::ip::tcp::endpoint tcp_client(boost::asio::ip::address::from_string("127.0.0.1"), client_port);
            m_mappings[client_port] = new TCPToUDPMapping(m_io_service, m_raw_sender, m_tcp_server, tcp_client);
        }

        m_mappings.at(client_port)->udp_sender().push(packet);
    }

private:
    boost::asio::io_service& m_io_service;
    boost::asio::ip::tcp::endpoint m_tcp_server;
    DisconnectedSender<boost::asio::generic::raw_protocol::socket> m_raw_sender;

    map<unsigned short, TCPToUDPMapping*> m_mappings;  // port -> mapping
};

class Client {
public:
    Client() : 
        m_tcp_server(boost::asio::ip::address::from_string("127.0.0.1"), 44401u),
        m_raw_socket(m_io_service, boost::asio::generic::raw_protocol(AF_INET, IPPROTO_TCP)),

        m_tcp_to_udp_router(m_raw_socket, m_tcp_server),
        m_filter(m_tcp_to_udp_router, m_tcp_server.port()),
        m_raw_receiver(m_raw_socket, m_filter, "raw receiver")
    {
    }

    void run() {
        cout << "running client" << endl;
        m_io_service.run();
    }

private:
    boost::asio::io_service m_io_service;
    boost::asio::ip::tcp::endpoint m_tcp_server;  // endpoint of the tcp server
    boost::asio::generic::raw_protocol::socket m_raw_socket;

    TCPToUDPRouter m_tcp_to_udp_router;
    TCPPortFilter m_filter;
    Receiver<boost::asio::generic::raw_protocol::socket> m_raw_receiver;
};

// TODO we can probably assume that upon each read exactly one packet is received, need to browse the web for that. We could switch to easier buffers too then, easier Packet formats too.
class Server {
public:
    Server() :
        m_raw_socket(m_io_service, boost::asio::generic::raw_protocol(AF_INET, IPPROTO_TCP)),
        m_udp_socket(m_io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), udp_port_s)),

        m_udp_sender(m_udp_socket, "udp sender"),
        m_filter(m_udp_sender, 44403u),
        m_raw_receiver(m_raw_socket, m_filter, "raw receiver"),

        m_raw_sender(m_raw_socket, "raw sender"),
        m_rewriter(m_raw_sender, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 44403u), boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 22u)),
        m_udp_receiver(m_udp_socket, m_rewriter, "udp receiver")
    {
    }

    void run() {
        m_udp_socket.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), udp_port_c));

        cout << "running server" << endl;
        m_io_service.run();
    }

private:
    boost::asio::io_service m_io_service;
    boost::asio::generic::raw_protocol::socket m_raw_socket;
    boost::asio::ip::udp::socket m_udp_socket;

    Sender<boost::asio::ip::udp::socket> m_udp_sender;
    TCPPortFilter m_filter;
    Receiver<boost::asio::generic::raw_protocol::socket> m_raw_receiver;

    DisconnectedSender<boost::asio::generic::raw_protocol::socket> m_raw_sender;
    Rewriter m_rewriter;
    Receiver<boost::asio::ip::udp::socket> m_udp_receiver;
};


/*
 * UDP Tunnel server main(). Handles program arguments, initializes everything,
 * and runs the main loop.
 */
int udpserver(int argc, char *argv[])
{
    try {
        signal(SIGINT, &signal_handler);

        UDT::startup();

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
            // TODO io_service.stop()
    }
}

