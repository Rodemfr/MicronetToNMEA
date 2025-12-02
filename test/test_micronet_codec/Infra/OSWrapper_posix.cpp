/**
 * @file OSWrapper_posix.cpp
 * @brief POSIX implementation of OS wrapper functions for native testing.
 *
 * This file provides a POSIX-compliant implementation of functions like millis(),
 * allowing code that depends on Arduino-specific functions to be compiled and
 * tested on a native host (e.g., Linux, macOS).
 */

#include "OSWrapper.h"
#include <time.h>

/**
 * @brief Returns the number of milliseconds since an arbitrary point in time.
 * @return The number of milliseconds as an unsigned long.
 */
unsigned long millis()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/**
 * @brief Pauses the program for the specified number of milliseconds.
 * @param ms The number of milliseconds to wait.
 */
void delay(uint32_t ms)
{
    struct timespec req;
    req.tv_sec  = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}