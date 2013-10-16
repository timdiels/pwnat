/*
 * Copyright (C) 2009, 2010, 2013 by Daniel Meekins, Samy Kamkar, Tim Diels
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

#include "common.h"

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <pwnat/accumulator.hpp>

#include "client/TCPServer.h"
#include "server/ProxyServer.h"

using namespace std;
namespace po = boost::program_options;

struct Args {
    bool is_server;
    bool is_ipv6; // if not, it's ipv4
    int verbosity;
    std::string bind_address; // TODO This has to do with binding of serving sockets. Defaults to 0.0.0.0 or ::, is an ip not a dns string
    u_int16_t proxy_port;

    // client only args
    u_int16_t local_port;
    std::string proxy_host; // TODO can be a dns string, resolve locally
    std::string remote_host; // TODO can be a dns string, resolve remotely
    u_int16_t remote_port;
};

void parse_args(int argc, char *argv[], Args& args);

int main(int argc, char *argv[]) {
    Args args;

    try {
        parse_args(argc, argv, args);
    }
    catch (const exception& e) {
        cerr << "Parse error: " << e.what() << endl;
        return 1;
    }

    try {
        if(!args.is_server) {
            // client
            TCPServer client;
            client.run();
        }
        else {
            // server
            ProxyServer server;
            server.run();
        }

    }
    catch (const exception& e) {
        cerr << e.what() << endl;
        abort();
    }

    cout << "Main thread exiting" << endl;
    return 0;
}

void print_usage(po::options_description& options_spec) {
    cout << "pwnat client-mode listens for TCP traffic client-side and forwards it to a pwnat proxy server." << endl
         << endl
         << "pwnat server-mode forwards data of connected proxy clients to the remote host specified by the client. pwnat server forwards replies to appropriate client. Multiple proxy and TCP clients are supported. Proxy client and server may both be behind a NAT." << endl
         << endl
         << "Synopsis:" << endl
         << "  pwnat -c <options> <local port> <proxy host> <remote host> <remote port>" << endl
         << "  pwnat -s <options>" << endl
         << endl
         << options_spec << endl;
}

void parse_args(int argc, char *argv[], Args& args) {
    po::options_description options_spec("General Options");
    options_spec.add_options()
        ("help,h", po::bool_switch(), "show help and exit")
        ("server,s", po::bool_switch(&args.is_server), "server mode")
        ("client,c", po::bool_switch(), "client mode")
        ("ipv6,6", po::bool_switch(&args.is_ipv6), "use IPv6")
        ("verbose,v", accumulator<int>(&args.verbosity)->implicit_value(1), "increase verbosity")
        ("bindaddress,b", po::value<string>(&args.bind_address)->default_value(""), "local IP to bind to")
        ("proxyport,p", po::value<u_int16_t>(&args.proxy_port)->default_value(2222), "proxy server port") // TODO is default noted in usage output?
    ;

    po::options_description client_specific_options("Client Options");
    client_specific_options.add_options()
        ("localport", po::value<u_int16_t>(&args.local_port), "local TCP port to listen on")
        ("proxyhost", po::value<string>(&args.proxy_host), "proxy host dns/ip")
        ("remotehost", po::value<string>(&args.remote_host), "remote server dns/ip, resolved on proxy server.")
        ("remoteport", po::value<u_int16_t>(&args.remote_port), "remote port")
    ;

    po::positional_options_description positional_options; // maps positional options to regular options
    positional_options
        .add("localport", 1)
        .add("proxyhost", 1)
        .add("remotehost", 1)
        .add("remoteport", 1)
    ;

    options_spec.add(client_specific_options);

    po::variables_map vars;
    po::store(po::command_line_parser(argc, argv).options(options_spec).positional(positional_options).run(), vars);
    po::notify(vars);

    if (vars["help"].as<bool>()) {
        print_usage(options_spec);
        exit(1);
    }

    if (!(vars["server"].as<bool>() xor vars["client"].as<bool>())) {
        throw runtime_error("Need to specify one of --server and --client");
    }

    // client only args
    if (!args.is_server) {
        if (!vars.count("localport")) {
            throw runtime_error("Need to specify --localport");
        }
        if (!vars.count("proxyhost")) {
            throw runtime_error("Need to specify --proxyhost");
        }
        if (!vars.count("remotehost")) {
            throw runtime_error("Need to specify --remotehost");
        }
        if (!vars.count("remoteport")) {
            throw runtime_error("Need to specify --remoteport");
        }
    }

    cout<< "serv" << args.is_server << endl 
        << "ipv6" << args.is_ipv6 << endl 
        << "v" << args.verbosity << endl 
        << "bind" << args.bind_address << endl 
        << "pport" << args.proxy_port << endl
        << "lport" << args.local_port << endl
        << "phost" << args.proxy_host << endl
        << "rhost" << args.remote_host << endl
        << "rport" << args.remote_port << endl;
}

