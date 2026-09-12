#include <stdint.h>
#include <string.h>

/* variables */
const int _test = 0x1234;
const char * _ptest = (char *)&_test;


/* functions */
/* htonl */
uint32_t htonl(uint32_t host32)
{
    if(*_ptest == 0x12)
        return host32;
    return ((host32 & 0xff) << 24) | ((host32 & 0xff00) << 8)
        | ((host32 & 0xff0000) >> 8) | ((host32 & 0xff000000) >> 24);
}


/* htons */
uint16_t htons(uint16_t host16)
{
    if(*_ptest == 0x12)
        return host16;
    return ((host16 & 0xff) << 8) | ((host16 & 0xff00) >> 8);
}


/* ntohl */
uint32_t ntohl(uint32_t net32)
{
    return htonl(net32);
}


/* ntohs */
uint16_t ntohs(uint16_t net16)
{
    return htons(net16);
}


//https://codereview.stackexchange.com/questions/149717/implementation-of-c-standard-library-function-ntohl
//https://github.com/joeferner/stm32-network/blob/master/interface.h