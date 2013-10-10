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

template <typename chartype>
class BasicPacket {
public:
    BasicPacket(chartype* data, size_t length) : 
        m_data(data),
        m_length(length)
    {
    }

    chartype* data() {
        return m_data;
    }

    size_t length() {
        return m_length;
    }


private:
    chartype* m_data;
    size_t m_length;
};

typedef BasicPacket<char> Packet;
typedef BasicPacket<const char> ConstPacket;

class NetworkPipe {
public:
    virtual ~NetworkPipe() {}

    /**
     * Push packet of given length onto pipe
     */
    virtual void push(ConstPacket&) = 0;
};
