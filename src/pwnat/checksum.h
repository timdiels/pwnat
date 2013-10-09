
#ifndef PACKET_T_H 
#define PACKET_T_H 

//	Includes
#ifndef WIN32
  	#include	<sys/unistd.h>
  	#include	<sys/types.h>
  	#include	<sys/socket.h>
  	#include	<netinet/in.h>
	#include 	<limits.h>
  	#include	<arpa/inet.h>
  	#include	<netdb.h>
	#include	<pthread.h>
	#include	<errno.h>
	#include	<syslog.h>
	#include	<pwd.h>
	#include	<grp.h>
#endif /* !WIN32 */
	#include	<stdarg.h>
	#include	<unistd.h>
  	#include	<stdio.h>
  	#include	<stdlib.h>
  	#include	<string.h>
  	#include	<time.h>
  	#include	<sys/time.h>
  	#include	<signal.h>
  	#include	<stdint.h>

#ifdef WIN32
	#include    <winsock2.h>
	typedef int socklen_t;
	typedef uint32_t in_addr_t;
#endif /* WIN32 */

uint16_t get_checksum(uint16_t *data, int bytes);

#endif
