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

#include <iostream>
#include "client/TCPServer.h"
#include "server/ProxyServer.h"

using namespace std;

int main(int argc, char *argv[]) {
    ProgramArgs args;

    try {
        args.parse(argc, argv);
    }
    catch (const exception& e) {
        cerr << "Parse error: " << e.what() << endl;
        return 1;
    }

    try {
        if(!args.is_server()) {
            // client
            TCPServer client(args);
            client.run();
        }
        else {
            // server
            ProxyServer server(args);
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
