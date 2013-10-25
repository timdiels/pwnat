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

#include "ProgramArgs.h"
#include <iostream>
#include <boost/asio.hpp>
#include <pwnat/accumulator.hpp>
#include <pwnat/checksum.h>
#include <pwnat/packet.h>
#include <boost/log/trivial.hpp>

#include <pwnat/namespaces.h>
namespace po = boost::program_options;

void ProgramArgs::parse(int argc, char *argv[]) {
    po::options_description options_spec("General Options");
    options_spec.add_options()
        ("help,h", po::bool_switch(), "show help and exit")
        ("server,s", po::bool_switch(&m_is_server), "server mode")
        ("client,c", po::bool_switch(), "client mode")
        ("ipv6,6", po::bool_switch(&m_is_ipv6), "use IPv6")
        ("verbose,v", accumulator<int>(&m_verbosity)->implicit_value(1), "increase verbosity")
        ("bindaddress,b", po::value<string>(), "local IP to bind to")
        ("proxyport,p", po::value<u_int16_t>(&m_proxy_port)->default_value(2222), "proxy server port")
    ;

    po::options_description client_specific_options("Client Options");
    client_specific_options.add_options()
        ("localport", po::value<u_int16_t>(&m_local_port), "local TCP port to listen on")
        ("proxyhost", po::value<string>(&m_proxy_host_dns), "proxy host dns/ip")
        ("remotehost", po::value<string>(&m_remote_host), "remote server dns/ip, resolved on proxy server.")
        ("remoteport", po::value<u_int16_t>(&m_remote_port), "remote port")
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

    if (vars.count("bindaddress")) {
        m_bind_address = asio::ip::address::from_string(vars["bindaddress"].as<string>());
    } else {
        m_bind_address = loopback();
    }

    // client only args
    if (!m_is_server) {
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
}

void ProgramArgs::resolve_proxy_host(boost::asio::io_service& io_service) {
    assert(!m_is_server);
    BOOST_LOG_TRIVIAL(debug) << "Resolving " << m_proxy_host_dns << endl;
    asio::ip::udp::resolver resolver(io_service);

    stringstream str;
    str << m_proxy_port;
    asio::ip::udp::resolver::query query(udp_version(), m_proxy_host_dns, str.str());
    m_proxy_host = resolver.resolve(query)->endpoint().address();
}

void ProgramArgs::print_usage(po::options_description& options_spec) {
    cout << endl
         << "pwnat client-mode listens for TCP traffic client-side and forwards it to a pwnat proxy server." << endl
         << endl
         << "pwnat server-mode forwards data of connected proxy clients to the remote host specified by the client. pwnat server forwards replies to appropriate client. Multiple proxy and TCP clients are supported. Proxy client and server may both be behind a NAT." << endl
         << endl
         << "Synopsis:" << endl
         << "  pwnat -c <options> <local port> <proxy host> <remote host> <remote port>" << endl
         << "  pwnat -s <options>" << endl
         << endl
         << options_spec << endl;
}

bool ProgramArgs::is_server() const {
    return m_is_server;
}

bool ProgramArgs::is_ipv6() const {
    return m_is_ipv6;
}

int ProgramArgs::verbosity() const {
    return m_verbosity;
}

const boost::asio::ip::address& ProgramArgs::bind_address() const {
    return m_bind_address;
}

u_int16_t ProgramArgs::proxy_port() const {
    return m_proxy_port;
}

u_int16_t ProgramArgs::local_port() const {
    return m_local_port;
}

const boost::asio::ip::address& ProgramArgs::proxy_host() const {
    return m_proxy_host;
}

const std::string& ProgramArgs::remote_host() const {
    return m_remote_host;
}

u_int16_t ProgramArgs::remote_port() const {
    return m_remote_port;
}

asio::ip::icmp ProgramArgs::icmp_version() const {
    if (m_is_ipv6) {
        return asio::ip::icmp::v6();
    }
    else {
        return asio::ip::icmp::v4();
    }
}

asio::ip::udp ProgramArgs::udp_version() const {
    if (m_is_ipv6) {
        return asio::ip::udp::v6();
    }
    else {
        return asio::ip::udp::v4();
    }
}

asio::ip::tcp ProgramArgs::tcp_version() const {
    if (m_is_ipv6) {
        return asio::ip::tcp::v6();
    }
    else {
        return asio::ip::tcp::v4();
    }
}

asio::ip::address ProgramArgs::loopback() const {
    if (m_is_ipv6) {
        return asio::ip::address_v6::loopback();
    }
    else {
        return asio::ip::address_v4::loopback();
    }
}

int ProgramArgs::address_family() const {
    if (m_is_ipv6) {
        return AF_INET6;
    }
    else {
        return AF_INET;
    }
}

asio::ip::address ProgramArgs::icmp_echo_destination() const {
    return loopback();
    /*if (m_is_ipv6) {
        return "::"; // TODO replace by ??
    }
    else {
        return "127.0.0.1"; // TODO replace by 3.3.3.3
    }*/
}

void ProgramArgs::get_icmp_echo(vector<char>& buffer, u_int16_t id, u_int16_t sequence) const {
    buffer.clear();
    if (m_is_ipv6) {
        // TODO fix checksum: http://en.wikipedia.org/wiki/ICMPv6#Message_checksum
        buffer.resize(sizeof(icmp6_hdr), 0);
        auto icmp = reinterpret_cast<icmp6_hdr*>(buffer.data());
        icmp->icmp6_type = ICMP6_ECHO_REQUEST;
        icmp->icmp6_id = htons(id);
        icmp->icmp6_seq = htons(sequence);
        icmp->icmp6_cksum = htons(get_checksum(reinterpret_cast<u_int16_t*>(buffer.data()), buffer.size()));
    }
    else {
        buffer.resize(sizeof(icmphdr), 0);
        auto icmp = reinterpret_cast<icmphdr*>(buffer.data());
        icmp->type = ICMP_ECHO;
        icmp->un.echo.id = htons(id);
        icmp->un.echo.sequence = htons(sequence);
        icmp->checksum = htons(get_checksum(reinterpret_cast<u_int16_t*>(buffer.data()), buffer.size()));
    }
}
