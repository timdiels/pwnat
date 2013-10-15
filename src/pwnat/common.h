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

#pragma once

#include <limits>

#define NO_DEBUG     0
#define DEBUG_LEVEL1 1
#define DEBUG_LEVEL2 2
#define DEBUG_LEVEL3 3

// TODO rm
extern int opt_debug;

#ifdef WIN32
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif

/* cl.exe has a different 'inline' keyword for some dumb reason */
#ifdef WIN32
#define _inline_ __inline
#else
#define _inline_ inline
#endif

// TODO rm
#define PERROR_GOTO(cond,err,label){        \
        if(cond)                            \
        {                                   \
                perror(err) ;               \
            goto label;                     \
        }}

            //if(debug_level >= DEBUG_LEVEL1)

#define ERROR_GOTO(cond,str,label){                  \
        if(cond)                                     \
        {                                            \
            if(debug_level >= DEBUG_LEVEL2)          \
                fprintf(stderr, "Error: %s\n", str); \
            goto label;                              \
        }}

// TODO rm
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
