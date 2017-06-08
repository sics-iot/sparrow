/*---------------------------------------------------------------------------*/
#include "ip64-null-interface.h"
/*---------------------------------------------------------------------------*/
#define IP64_CONF_UIP_FALLBACK_INTERFACE ip64_null_interface
#define IP64_CONF_INPUT                  ip64_null_interface_input
#include "ip64-native-driver.h"
#define IP64_CONF_ETH_DRIVER             ip64_native_driver
