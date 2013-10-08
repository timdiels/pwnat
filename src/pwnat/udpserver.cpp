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
#include <boost/array.hpp>
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

template <typename chartype>
class BasicPacket {
public:
    BasicPacket(chartype* data, size_t length) : 
        m_data(data),
        m_length(length)
    {
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

typedef BasicPacket<char> Packet;
typedef BasicPacket<const char> ConstPacket;

class NetworkPipe {
public:
    /**
     * Push packet of given length onto pipe
     */
    virtual void push(ConstPacket&) = 0;
};

/**
 * Receives from socket and pushes to a NetworkPipe
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
        boost::array<char, IP_MAXPACKET> buffer;
        while (true) {
            try {
                size_t bytes_transferred = m_socket.async_receive(boost::asio::buffer(buffer), yield);
                cout << m_name << " received " << bytes_transferred << endl;
                ConstPacket packet(buffer.data(), bytes_transferred); // TODO can't we just hand it to a string object or such?
                m_pipe.push(packet);
            }
            catch (std::exception& e) {
                cerr << m_name << ": receive error: " << e.what() << endl;
                abort();
            }
        }
    }

private:
    Socket& m_socket;
    NetworkPipe& m_pipe;
    string m_name;
};
typedef Receiver<boost::asio::ip::tcp::socket> TCPReceiver;

/**
 * Receives from UDT socket and pushes to a NetworkPipe
 */
class UDTReceiver {
public:
    UDTReceiver(UDTSOCKET& socket, NetworkPipe& pipe, string name) : 
        m_socket(socket),
        m_pipe(pipe),
        m_name(name)
    {
    }

    void receive(UDTSOCKET& socket) { // TODO async, duh
        m_socket = socket;
        boost::array<char, IP_MAXPACKET> buffer;
        while (true) {
            int bytes_transferred = UDT::recv(m_socket, buffer.data(), buffer.size(), 0);
            if (bytes_transferred == UDT::ERROR) {
                cerr << m_name << ": error: " << UDT::getlasterror().getErrorMessage() << endl;
                abort();
            }
            else {
                cout << m_name << " received " << bytes_transferred << endl;
                ostream ostr(&m_received);
                ostr.write(buffer.data(), bytes_transferred);
                push_received();  // TODO io_stream dispatch push_received
            }
        }
    }

//TODO private:
    /**
     * Push everything we've received so far
     */
    void push_received() {
        // TODO lock
        if (m_received.size() > 0) {
            ConstPacket packet(boost::asio::buffer_cast<const char*>(m_received.data()), m_received.size());
            m_pipe.push(packet);
            m_received.consume(packet.length());
        }
        // TODO unlock
    }

private:
    boost::asio::streambuf m_received;  // buffer which contains what has been received, but not yet pushed to m_pipe
    UDTSOCKET m_socket;
    //UDTSOCKET& m_socket; TODO
    NetworkPipe& m_pipe;
    string m_name;
};

/**
 * Sends pushed data to connected socket
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
typedef Sender<boost::asio::ip::tcp::socket> TCPSender;

/**
 * Sends pushed packets to connected UDT socket
 */
class UDTSender : public NetworkPipe {
public:
    UDTSender(UDTSOCKET& socket, string name) :
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
    UDTSOCKET& m_socket;
    string m_name;
};

class TCPClient {
public:
    TCPClient(boost::asio::ip::tcp::socket* tcp_socket) :
        m_tcp_socket(tcp_socket),
        m_udt_socket(UDT::socket(AF_INET, SOCK_STREAM, 0)),

        m_udt_sender(m_udt_socket, "udt sender"),
        m_tcp_receiver(*m_tcp_socket, m_udt_sender, "tcp receiver") // TODO name + tcp client port, or no names, or name defaults to: proto sender/receiver src->dst port (let's do that latter)
    {
        assert(m_udt_socket != UDT::INVALID_SOCK); // TODO exception throw

        sockaddr_in localhost;
        localhost.sin_family = AF_INET;
        localhost.sin_addr.s_addr = inet_addr("127.0.0.1");
        memset(&localhost.sin_zero, 0, 8);

        localhost.sin_port = htons(udp_port_c);
        if (UDT::ERROR == UDT::bind(m_udt_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
            cerr << "client bind error: " << UDT::getlasterror().getErrorMessage() << endl;
            abort();
        }

        localhost.sin_port = htons(udp_port_s);
        if (UDT::ERROR == UDT::connect(m_udt_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
            cerr << "client connect error: " << UDT::getlasterror().getErrorMessage() << endl;
            abort();
        }
    }

    ~TCPClient() {
        delete m_tcp_socket;
    }

private:
    boost::asio::ip::tcp::socket* m_tcp_socket;
    UDTSOCKET m_udt_socket;

    UDTSender m_udt_sender;
    TCPReceiver m_tcp_receiver;
};

class ClientTCPServer {
public:
    ClientTCPServer(boost::asio::io_service& io_service) :
        m_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 44401u))
    {
        accept();
    }

    void accept() {
        auto new_socket = new boost::asio::ip::tcp::socket(m_acceptor.get_io_service());
        auto callback = boost::bind(&ClientTCPServer::handle_accept, this, boost::asio::placeholders::error, new_socket);
        m_acceptor.async_accept(*new_socket, callback);
    }

    void handle_accept(const boost::system::error_code& error, boost::asio::ip::tcp::socket* tcp_socket) {
        if (error) {
            cerr << "TCP Server: accept error: " << error.message() << endl;
        }
        else {
            cout << "New tcp client at port " << tcp_socket->remote_endpoint().port() << endl;
            new TCPClient(tcp_socket);  // Note: ownership of socket transferred to TCPClient instance
        }

        accept();
    }

private:
    boost::asio::ip::tcp::acceptor m_acceptor;
};

class Client {
public:
    Client() :
        m_tcp_server(m_io_service)
    {
    }

    void run() {
        cout << "running client" << endl;
        m_io_service.run();
    }

private:
    boost::asio::io_service m_io_service;
    ClientTCPServer m_tcp_server;
};

class Server {
public:
    Server() :
        m_raw_socket(m_io_service),
        m_udt_socket(UDT::socket(AF_INET, SOCK_STREAM, 0)),

        //m_udp_sender(m_udp_socket, "udp sender"),
        //m_filter(m_udp_sender, 44403u),
        //m_raw_receiver(m_raw_socket, m_filter, "raw receiver"),

        m_raw_sender(m_raw_socket, "raw sender"),
        m_udt_receiver(m_udt_socket, m_raw_sender, "udt receiver")
    {
        // TCP
        m_raw_socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 22u)); // TODO async when udt client connects

        // UDT
        assert(m_udt_socket != UDT::INVALID_SOCK); // todo exception throw

        sockaddr_in localhost; // TODO rename udt_server
        localhost.sin_family = AF_INET;
        localhost.sin_addr.s_addr = inet_addr("127.0.0.1");
        localhost.sin_port = htons(udp_port_s);
        memset(&localhost.sin_zero, 0, 8);

        if (UDT::ERROR == UDT::bind(m_udt_socket, reinterpret_cast<sockaddr*>(&localhost), sizeof(sockaddr_in))) {
            cerr << "udt bind error: " << UDT::getlasterror().getErrorMessage() << endl;
            abort();
        }

        if (UDT::ERROR == UDT::listen(m_udt_socket, 10)) {
            cerr << "udt listen error: " << UDT::getlasterror().getErrorMessage() << endl;
            abort();
        }

        // TODO rendezvous (hole punching) works without listen/accept, it's a connect to each other thing!
        UDTSOCKET udt_client_socket = UDT::accept(m_udt_socket, nullptr, nullptr);
        if (udt_client_socket == UDT::INVALID_SOCK) {
            cerr << "udt accept error: " << UDT::getlasterror().getErrorMessage() << endl;
            abort();
        }

        m_udt_receiver.receive(udt_client_socket);
    }

    void run() {
        cout << "running server" << endl;
        m_io_service.run();
    }

private:
    boost::asio::io_service m_io_service;

    boost::asio::ip::tcp::socket m_raw_socket;
    UDTSOCKET m_udt_socket;

    /*Sender<boost::asio::ip::udp::socket> m_udp_sender;
    TCPPortFilter m_filter;
    Receiver<boost::asio::generic::raw_protocol::socket> m_raw_receiver;*/

    TCPSender m_raw_sender; // TODO rename
    UDTReceiver m_udt_receiver;
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

