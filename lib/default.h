#ifndef DEFAULT_H
#define DEFAULT_H
#include <util/delay.h>

#define SET(x,y) (x|=(1<<y))
#define CLR(x,y) (x&=(~(1<<y)))
#define CHK(x,y) (x&(1<<y)) 
#define TOG(x,y) (x^=(1<<y))
#define Delay(x) (_delay_us(x))

#define MAX_INT8 127
#define MAX_UINT8 0xFF
#define MAX_INT16 32767
#define MAX_UINT16 0xFFFF
#define MAX_INT32 2147483647
#define MAX_UINT32 0xFFFFFFFF

#define MIN_INT8 (-128)
#define MIN_INT16 (-32767)
#define MIN_INT32 (-2147483647)

#define gravity 10


#endif