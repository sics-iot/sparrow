#ifndef IP64_NULL_INTERFACE_H
#define IP64_NULL_INTERFACE_H

#include "net/ip/uip.h"

void ip64_null_interface_input(uint8_t *packet, uint16_t len);

extern const struct uip_fallback_interface ip64_null_interface;

#endif /* IP64_NULL_INTERFACE_H */
