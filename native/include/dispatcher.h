// sdcdcd
#pragma once
#include <cstdint>
void dispatch_command(uint8_t command, const uint8_t *req_body,
                      uint32_t req_len, uint8_t *resp_body, uint32_t resp_cap,
                      uint32_t *resp_len);
