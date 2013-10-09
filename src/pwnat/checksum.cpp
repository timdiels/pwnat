
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#else
#endif

#include "common.h"
#include "checksum.h"

uint16_t get_checksum(uint16_t *data, int bytes)
{
    uint32_t sum;
    int i;

    sum = 0;
    for (i=0;i<bytes/2;i++) {
            sum += data[i];
    }
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum = htons(0xFFFF - sum);
    return sum;
}

