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
#include <boost/program_options.hpp>

/**
 * The configuration of the program
 */
class ProgramArgs {
public:
    void parse(int argc, char *argv[]);
    void resolve_proxy_host(boost::asio::io_service& io_service); // needs to be called exactly once before using proxy_host()

    bool is_server() const;
    bool is_ipv6() const;
    int verbosity() const;
    const boost::asio::ip::address& bind_address() const;
    u_int16_t proxy_port() const;

    u_int16_t local_port() const;
    const boost::asio::ip::address& proxy_host() const;
    const std::string& remote_host() const;
    u_int16_t remote_port() const;

    boost::asio::ip::icmp icmp_version() const;
    boost::asio::ip::udp udp_version() const;
    boost::asio::ip::tcp tcp_version() const;
    boost::asio::ip::address loopback() const;
    int address_family() const;

    boost::asio::ip::address icmp_echo_destination() const;
    void get_icmp_echo(std::vector<char>& buffer, u_int16_t id, u_int16_t sequence) const;

private:
    void print_usage(boost::program_options::options_description& options_spec);

private:
    bool m_is_server;
    bool m_is_ipv6; // if false, it's ipv4
    int m_verbosity;
    boost::asio::ip::address m_bind_address;
    u_int16_t m_proxy_port;

    u_int16_t m_local_port;
    boost::asio::ip::address m_proxy_host;
    std::string m_proxy_host_dns;
    std::string m_remote_host;
    u_int16_t m_remote_port;
};

