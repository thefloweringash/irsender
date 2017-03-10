// -*- c++ -*-

#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>

template <typename T>
T read_unaligned(const uint8_t *data) {
    T result;
    memcpy(&result, data, sizeof(T));
    return result;
}

#endif
