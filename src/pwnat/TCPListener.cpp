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

extern int debug_level;

#include "TCPListener.h"
#include <cstdlib>
#include <ctime>

using namespace std;

TCPListener::TCPListener(char *host, char *port, char* proxy_host, char* proxy_port, char* remote_host, char* remote_port, int ipver) 
    : ipver(ipver), phost(proxy_host), pport(proxy_port), rhost(remote_host), rport(remote_port)
{
    /* Create a TCP server socket to listen for incoming connections */
    tcp_serv = sock_create(host, port, ipver, SOCK_TYPE_TCP, 1, 1);
    if(debug_level >= DEBUG_LEVEL1)
    {
        char addrstr[ADDRSTRLEN];
        printf("Listening on TCP %s\n", sock_get_str(tcp_serv, addrstr, sizeof(addrstr)));
    }

    srand(time(0));
    next_req_id = rand() % 0xffff;
}

TCPListener::~TCPListener() {
    sock_close(tcp_serv);
    sock_free(tcp_serv);
}

/* Check if pending TCP connection to accept and create a new client
   and UDP connection if one is ready */
void TCPListener::poll(vector<client_t*> *conn_clients, fd_set& client_fds, fd_set& read_fds, int& num_fds) {
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
