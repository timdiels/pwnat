/*
 * Code taken from: http://sysnet.ucsd.edu/~cfleizac/iptcphdr.html
 * and modified slightly.
 */

#include "checksum.h"
#include <cstring>

// thanx to http://seclists.org/lists/bugtraq/1999/Mar/0057.html
struct tcp_pseudo /*the tcp pseudo header*/
{
    u_int32_t src_addr;
    u_int32_t dst_addr;
    u_int8_t zero;
    u_int8_t proto;
    u_int16_t length;
} pseudohead;

void set_tcp_checksum(struct ip* myip, struct tcphdr * mytcp) {
    mytcp->check = 0;
    u_int16_t total_len = ntohs(myip->ip_len);

    int tcpopt_len = mytcp->doff*4 - 20;
    int tcpdatalen = total_len - (mytcp->doff*4) - (myip->ip_hl*4);

    pseudohead.src_addr=myip->ip_src.s_addr;
    pseudohead.dst_addr=myip->ip_dst.s_addr;
    pseudohead.zero=0;
    pseudohead.proto=IPPROTO_TCP;
    pseudohead.length=htons(sizeof(struct tcphdr) + tcpopt_len + tcpdatalen);

    int totaltcp_len = sizeof(struct tcp_pseudo) + sizeof(struct tcphdr) + tcpopt_len + tcpdatalen;
    unsigned short * tcp = new unsigned short[totaltcp_len];

    memcpy((unsigned char *)tcp,&pseudohead,sizeof(struct tcp_pseudo));
    memcpy((unsigned char *)tcp+sizeof(struct tcp_pseudo),(unsigned char *)mytcp,sizeof(struct tcphdr));
    memcpy((unsigned char *)tcp+sizeof(struct tcp_pseudo)+sizeof(struct tcphdr), (unsigned char *)myip+(myip->ip_hl*4)+(sizeof(struct tcphdr)), tcpopt_len);
    memcpy((unsigned char *)tcp+sizeof(struct tcp_pseudo)+sizeof(struct tcphdr)+tcpopt_len, (unsigned char *)mytcp+(mytcp->doff*4), tcpdatalen);

    mytcp->check = checksum(tcp, totaltcp_len);
}

long checksum(unsigned short *addr, unsigned int count) {
    /* Compute Internet Checksum for "count" bytes
     *         beginning at location "addr".
     */
    register long sum = 0;

    while( count > 1 )  {
        /*  This is the inner loop */
        sum += * addr++;
        count -= 2;
    }

    /*  Add left-over byte, if any */
    if( count > 0 )
        sum += * (unsigned char *) addr;

    /*  Fold 32-bit sum to 16 bits */
    while (sum>>16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

