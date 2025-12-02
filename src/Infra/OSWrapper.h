#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C"
{
#endif

    unsigned long millis();
    void          delay(uint32_t ms);

#ifdef __cplusplus
}
#endif
