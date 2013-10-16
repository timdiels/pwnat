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

#include "client/TCPServer.h"
#include "server/ProxyServer.h"

using namespace std;

/*
 * UDP Tunnel server main(). Handles program arguments, initializes everything,
 * and runs the main loop.
 */
int udpserver(int argc, char *argv[])
{
    try {
        // prototype code
        if (argc == 0) {
            // pretend to be client
            TCPServer client;
            client.run();
        }
        else {
            // pretend to be server
            ProxyServer server;
            server.run();
        }

    } catch (exception& e) {
        cerr << e.what() << endl;
        abort();
    }

    cout << "Main thread exiting" << endl;
    return 0;
}

