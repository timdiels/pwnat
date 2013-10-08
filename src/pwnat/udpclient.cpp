/*
 * Copyright (C) 2009, 2013 by Daniel Meekins, Tim Diels
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

#include <string>
#include <vector>
#include <algorithm>
#include <signal.h>
#include <sys/types.h>

using namespace std;

static int running = 1;


static void signal_handler(int sig);

bool isnumber(const char* str) {
    if (!str) {
        return false;
    }

    char* end;
    strtol(str, &end, 10);
    return *end == '\0';
}

/*
 * argv: [local ip] <local port> <proxy host> [proxy port] <remote host> <remote port>
 */
int udpclient(int argc, char* argv[])
{
    //srand(time(0));

    signal(SIGINT, &signal_handler);

    /*char *lhost, *lport, *phost, *pport, *rhost, *rport;
    char data[MSG_MAX_LEN];
    char pport_s[6] = "2222";
    

    // Parse arguments
    int i = 0;    
    if (!isnumber(argv[i]))
        lhost = argv[i++];
    else	
        lhost = NULL;
    lport = argv[i++];
    phost = argv[i++];
    if (isnumber(argv[i]))
        pport = argv[i++];
    else	
        pport = pport_s;
    rhost = argv[i++];
    rport = argv[i++];*/

    ///////////////////////////////////////////////
    /*try {
        boost::asio::io_service io_service;
        TCPListener tcp_listener(io_service, lhost, lport, phost, pport, rhost, rport, ipver);
        io_service.run();
    }
    catch (exception& e)
    {
        cerr << e.what() << endl;
    }*/
    ///////////////////////////////////////////////

    while (running) {
    }
}


void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGINT:
            running = 0;
    }
}
