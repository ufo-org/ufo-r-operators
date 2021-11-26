#include "bitbuffer.h"

BitBuffer *BitBuffer_new(size_t max_bytes) {
    BitBuffer *buffer = malloc(sizeof(BitBuffer));

    if (buffer == NULL) {
        REPORT("Cannot allocate BitBuffer.");
        return NULL;
    }

    buffer->data = (unsigned char *) calloc(max_bytes, sizeof(unsigned char));
    buffer->current_bit = 0;
    buffer->current_byte = 0;
    buffer->max_bytes = max_bytes;

    if (buffer->data == NULL) {
        REPORT("Cannot allocate internal buffer in BitBuffer.");
        free(buffer);
        return NULL;
    }

    // Just in case.
    memset(buffer->data, 0, max_bytes);

    return buffer;
}

void BitBuffer_free(BitBuffer *buffer) {
    free(buffer->data);
    free(buffer);
}

int BitBuffer_append_bit(BitBuffer* buffer, unsigned char bit) {
    assert(buffer->current_bit < 8);
    if (buffer->data == NULL) {
        REPORT("BitBuffer has uninitialized data.\n");
        return -2;
    }

    // Bit twiddle the new bit into position
    buffer->data[buffer->current_byte] |= bit << (7 - buffer->current_bit);

    // If overflow bit, bump bytes. This goes first.
    if (buffer->current_bit == 7) {
        buffer->current_byte += 1;
        if (buffer->current_bit >= buffer->max_bytes) {
            REPORT("ERROR: BitBuffer overflow.\n");
            return -1;
        }
    }

    // Bump bits. 
    buffer->current_bit = (buffer->current_bit + 1) % 8;

    // We're done?    
    return 0;
}

int BitBuffer_append_byte(BitBuffer* buffer, unsigned char byte) {
    for (unsigned char i = 0; i < 8; i++) {
        unsigned char mask = 1 << (8 - 1 - i);
        unsigned char bit = (byte & mask) > 0 ? 1 : 0;
        int result = BitBuffer_append_bit(buffer, bit);
        if (result < 0) {
            return result;
        }    
    }
    return 0;
}

int BitBuffer_append_uint32(BitBuffer* buffer, uint32_t value) {
    for (unsigned char i = 0; i < 32; i++) {
        uint32_t mask = 1 << (32 - 1 - i);
        uint32_t bit = (value & mask) > 0 ? 1 : 0;
        int result = BitBuffer_append_bit(buffer, bit);
        if (result < 0) {
            return result;
        }    
    }
    return 0;
}