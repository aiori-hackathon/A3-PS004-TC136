#pragma once

// where stunnel is reachable: set per mode in platformio.ini ([env:lab] / [env:pi])
#if !defined(GATEWAY_IP) || !defined(GATEWAY_PORT)
#error "no gateway set: build with  pio run -e lab  or  pio run -e pi"
#endif

// the name in the gateway cert's SAN; checked during the handshake
#define GATEWAY_NAME  "gateway.lab.test"
