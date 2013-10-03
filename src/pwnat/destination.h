/*
 * Copyright (C) 2009 by Daniel Meekins
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

#ifndef DESTINATION_H
#define DESTINATION_H

#include <stddef.h>

typedef struct destination {
        const char *host;
        const char *port;
        char *data;
} destination_t;

destination_t *destination_create(const char *address);

#endif
