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
#include "socket.h" // TODO rename file: util.h
//#include "checksum.h" // TODO rm file
#include "NetworkPipe.h"
#include "UDTService.h"
#include "UDTSocket.h"

using namespace std;

const int udp_port_c = 22201;
const int udp_port_s = 22203;

void signal_handler(int sig);

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

class TCPClient {
public:
    TCPClient(UDTService& udt_service, boost::asio::ip::tcp::socket* tcp_socket) :
        m_tcp_socket(tcp_socket),

        m_tcp_sender(*m_tcp_socket, "tcp sender"),
        m_udt_socket(udt_service, udp_port_c, udp_port_s, m_tcp_sender),

        m_tcp_receiver(*m_tcp_socket, m_udt_socket, "tcp receiver") // TODO name + tcp client port, or no names, or name defaults to: proto sender/receiver src->dst port (let's do that latter)
    {
        
    }

    ~TCPClient() {
        delete m_tcp_socket;
    }

private:
    boost::asio::ip::tcp::socket* m_tcp_socket;
    TCPSender m_tcp_sender;
    UDTSocket m_udt_socket;
    TCPReceiver m_tcp_receiver;
};

class ClientTCPServer {
public:
    ClientTCPServer(boost::asio::io_service& io_service) :
        m_udt_service(io_service),
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
            new TCPClient(m_udt_service, tcp_socket);  // Note: ownership of socket transferred to TCPClient instance
        }

        accept();
    }

private:
    UDTService m_udt_service;
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
        m_udt_service(m_io_service),
        m_tcp_socket(m_io_service),

        m_tcp_sender(m_tcp_socket, "raw sender"),
        m_udt_socket(m_udt_service, udp_port_s, udp_port_c, m_tcp_sender),

        m_tcp_receiver(m_tcp_socket, m_udt_socket, "raw receiver")
    {
        // TCP
        m_tcp_socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 22u)); // TODO async when udt client connects
    }

    void run() {
        cout << "running server" << endl;
        m_io_service.run();
    }

private:
    boost::asio::io_service m_io_service;
    UDTService m_udt_service;

    boost::asio::ip::tcp::socket m_tcp_socket;

    TCPSender m_tcp_sender;
    UDTSocket m_udt_socket;
    TCPReceiver m_tcp_receiver;
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

