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

#include "Disposable.h"

Disposable::Disposable() :
    m_disposed(false)
{
}

Disposable::~Disposable() {
}

bool Disposable::dispose() {
    if (m_disposed) {
        return false;
    }
    else {
        m_disposed = true;
        return true;
    }
}

bool Disposable::disposed() {
    return m_disposed;
}
