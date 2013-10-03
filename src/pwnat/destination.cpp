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

#include <stdlib.h>
#include <string.h>

#include "destination.h"

destination_t *destination_create(const char *address)
{
    destination_t *dst;
    char *cp;

    dst = new destination_t;
    if (!dst)
        goto fail;
    memset(dst, 0, sizeof(destination_t));

    dst->data = strdup(address);
    if (!dst->data)
        goto fail;

    /* Break address into both host and port, both optional */
    cp = strchr(dst->data, ':');
    if (!cp)
    {
        dst->host = dst->data;
        dst->port = NULL;
    }
    else if (cp == dst->data)
    {
        dst->host = NULL;
        dst->port = cp + 1;
    }
    else
    {
        *cp = '\0';
        dst->host = dst->data;
        dst->port = cp + 1;
    }

    return dst;

fail:
    return NULL;
}
