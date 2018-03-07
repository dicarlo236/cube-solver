#ifndef _serial
#define _serial

#include <stdint.h>

void init_serial(const char* name);
void send_sequence(uint8_t* seq, int size);
void set_rts_pin(uint8_t on);


#endif
