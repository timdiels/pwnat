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

/*
#include "TCPListener.h"
#include <cstdlib>
#include <ctime>

using namespace std;

TCPListener::TCPListener(boost::asio::io_service& io_service, char *host, char *port, char* proxy_host, char* proxy_port, char* remote_host, char* remote_port, int ipver) 
    : acceptor(io_service, boost::asio::ip::tcp::endpoint(ipver, port), ipver(ipver), phost(proxy_host), pport(proxy_port), rhost(remote_host), rport(remote_port)
{
}

TCPListener::~TCPListener() {
}

void TCPListener::start_accept() {
    TCPClient::pointer new_connection = tcp_connection::create(acceptor.get_io_service());
    auto callback = boost::bind(&TCPListener::handle_accept, this, new_connection, boost::asio::placeholders::error);
    acceptor.async_accept(new_connection->socket(), callback);
}

*/
/* create a new client and UDP connection*/
/*
void TCPListener::handle_accept(TCPClient::pointer new_connection, const boost::system::error_code& error) {
    if (!error) {
        new_connection->start();
    }

    start_accept();
}

{
    static uint16_t next_req_id = rand() % 0xffff;
    if(FD_ISSET(SOCK_FD(tcp_serv), &read_fds))
    {
        socket_t *tcp_sock = sock_accept(tcp_serv);
        socket_t *udp_sock = sock_create(phost, pport, ipver, SOCK_TYPE_UDP, 0, 1);

        client_t* client = client_create(next_req_id++, tcp_sock, udp_sock, 1);
        if(!client || !tcp_sock || !udp_sock)
        {
            if(tcp_sock)
                sock_close(tcp_sock);
            if(udp_sock)
                sock_close(udp_sock);
        }
        else
        {
            conn_clients->push_back(client);
            
            client_send_hello(client, rhost, rport, CLIENT_ID(client));
            client_add_tcp_fd_to_set(client, &client_fds);
            client_add_udp_fd_to_set(client, &client_fds);
        }
        
        sock_free(tcp_sock);
        sock_free(udp_sock);

        num_fds--;
    }
}
*/
