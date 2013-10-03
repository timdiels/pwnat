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

class Client {
public:
    Client() :
        udp_socket(io_service),
        udp_receive_buffer(udp_buffer.prepare(1024)), // TODO need to redo prepare each time? Apparently we do...
        raw_socket(io_service, boost::asio::generic::raw_protocol(AF_INET, IPPROTO_TCP))
    {
    }

    void run() {
        udp_socket = boost::asio::ip::udp::socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), udp_port_c)),
        udp_socket.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), udp_port_s));

        receive_raw();
        receive_udp();

        cout << "running client" << endl;
        io_service.run();
    }

    void receive_raw() {
        auto callback = boost::bind(&Client::handle_receive_raw, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        raw_socket.async_receive(raw_buffer.prepare(65536), callback);
    }

    void receive_udp() {
        auto callback = boost::bind(&Client::handle_receive_udp, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        udp_socket.async_receive(udp_receive_buffer, callback);
    }

    void write_raw_to_udp() {
        auto callback = boost::bind(&Client::handle_write_raw, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        udp_socket.async_send(raw_buffer.data(), callback);
    }

    void write_udp_to_raw() {
        auto callback = boost::bind(&Client::handle_write_udp, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred);
        raw_socket.async_send(udp_buffer.data(), callback);
    }

    void handle_receive_raw(const boost::system::error_code& error, size_t bytes_transferred) {
        if (error) {
            cerr << "E receive raw" << endl;
        }
        else {
            raw_buffer.commit(bytes_transferred);

            if (raw_buffer.size() == bytes_transferred) {
            char data[IP_MAXPACKET];  // = max packet size
            istream is(&raw_buffer);
            is.read(data, 4);
            ip* ip_hdr = reinterpret_cast<ip*>(data);
            if (ip_hdr->ip_v != 4) {
                cerr << "Not ipv4: " << ip_hdr->ip_v << endl;
                abort();
            }
            is.read(data+4, ntohs(ip_hdr->ip_len) - 4);
            if (ip_hdr->ip_p != IPPROTO_TCP) {
                cerr << "Not TCP: " << ip_hdr->ip_p << endl;
                abort();
            }

            tcphdr* tcp_hdr = reinterpret_cast<tcphdr*>(data + ip_hdr->ip_hl * 4);

            if (ntohs(tcp_hdr->dest) == tcp_port_c) {
                cout << "received raw" << endl;
            }

                write_raw_to_udp();
            }  // else a write is already happening
            //print_hexdump(data, bytes_transferred);
        }

        receive_raw();
    }

    void handle_receive_udp(const boost::system::error_code& error, size_t bytes_transferred) {
        if (error) {
            cerr << "E receive udp" << endl;
        }
        else {
            cout << "received udp" << endl;
            udp_buffer.commit(bytes_transferred);
            dump_back(udp_buffer, bytes_transferred);
            if (udp_buffer.size() == bytes_transferred) {
                write_udp_to_raw();
            }  // else a write is already happening
        }

        receive_udp();
    }

    void handle_write_raw(const boost::system::error_code& error, size_t bytes_transferred) {
        if (error) {
            cerr << "E send raw to udp" << endl;
        }
        else {
            cout << "sent raw to udp" << endl;
            dump_front(raw_buffer, bytes_transferred);
            raw_buffer.consume(bytes_transferred);
            if (raw_buffer.size() > 0) {
                write_raw_to_udp();
            }
        }
    }

    void handle_write_udp(const boost::system::error_code& error, size_t bytes_transferred) {
        if (error) {
            cerr << "E send udp to raw" << endl;
        }
        else {
            cout << "sent udp to raw" << endl;
            dump_front(udp_buffer, bytes_transferred);
            udp_buffer.consume(bytes_transferred);
            if (udp_buffer.size() > 0) {
                write_udp_to_raw();
            }
        }
    }

protected:
    boost::asio::io_service io_service;

    boost::asio::ip::udp::socket udp_socket;
    boost::asio::streambuf udp_buffer; // udp to raw buffer
    boost::asio::streambuf::mutable_buffers_type udp_receive_buffer;

    boost::asio::generic::raw_protocol::socket raw_socket;
    boost::asio::streambuf raw_buffer;

    boost::asio::streambuf raw_to_udp_buffer;
};



class Server : public Client {
public:
    Server() :
        Client()
    {
    }

    void run() {
        udp_socket = boost::asio::ip::udp::socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), udp_port_s));

        udp_socket.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), udp_port_c));
        //raw_socket.connect(boost::asio::generic::raw_protocol::endpoint(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 22)));

        //receive_raw();
        receive_udp();

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

