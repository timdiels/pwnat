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

#include <vector>
#include "client.h"
#include "socket.h"

class TCPListener {
public:
    TCPListener(char *host, char *port, char* proxy_host, char* proxy_port, char *remote_host, char *remote_port, int ipver);
    ~TCPListener();

    void poll(std::vector<client_t*> *conn_clients, fd_set& client_fds, fd_set& read_fds, int& num_fds);

    socket_t *tcp_serv;
private:
    int ipver;
    char *phost;
    char *pport;
    char *rhost;
    char *rport;

    uint16_t next_req_id;
};

