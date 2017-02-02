/*---------------------------------------------------------------------------*/
#include "ip64-eth-interface.h"
/*---------------------------------------------------------------------------*/
#define IP64_CONF_UIP_FALLBACK_INTERFACE ip64_eth_interface
#define IP64_CONF_INPUT                  ip64_eth_interface_input
#include "ip64-tap-driver.h"
#define IP64_CONF_ETH_DRIVER             ip64_tap_driver
