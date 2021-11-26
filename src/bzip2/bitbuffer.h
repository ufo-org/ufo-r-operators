#pragma once

#include <stdint.h>
#include <stdlib.h>

// This is a buffer to which bytes or individuaL bits can both be appended.
typedef struct {
    unsigned char *data;
    uint32_t       current_bit;
    size_t         current_byte;
    size_t         max_bytes;
} BitBuffer;

BitBuffer *BitBuffer_new(size_t max_bytes);
void BitBuffer_free(BitBuffer *buffer);
int BitBuffer_append_bit(BitBuffer* buffer, unsigned char bit);
int BitBuffer_append_byte(BitBuffer* buffer, unsigned char byte);
int BitBuffer_append_uint32(BitBuffer* buffer, uint32_t value);