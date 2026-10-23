#pragma once

// where stunnel is reachable (the laptop for now, the Pi later)
#define GATEWAY_IP    "10.252.99.72"
#define GATEWAY_PORT  8530
// the name in the gateway cert's SAN; checked during the handshake
#define GATEWAY_NAME  "gateway.lab.test"
