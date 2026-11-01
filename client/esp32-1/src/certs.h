#pragma once

// files from certs/, built into the firmware (see src/CMakeLists.txt)
extern const char ca_crt_start[]     asm("_binary_ca_crt_start");
extern const char ca_crt_end[]       asm("_binary_ca_crt_end");
extern const char client_crt_start[] asm("_binary_esp32_1_crt_start");
extern const char client_crt_end[]   asm("_binary_esp32_1_crt_end");
extern const char client_key_start[] asm("_binary_esp32_1_key_start");
extern const char client_key_end[]   asm("_binary_esp32_1_key_end");
