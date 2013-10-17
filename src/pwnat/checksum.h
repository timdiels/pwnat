
#pragma once

//	Includes
#ifndef WIN32
  	#include	<netinet/in.h>
#endif /* !WIN32 */

uint16_t get_checksum(uint16_t *data, int bytes);

