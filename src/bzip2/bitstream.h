#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "shift.h"

typedef struct {
    FILE*         handle;
    uint32_t      buffer;
    uint32_t      buffer_live;
    const char   *path;
    uint64_t      read_bits;    
    ShiftRegister shift_register;
} FileBitStream;

FileBitStream *FileBitStream_new(const char *input_file_path);

// Returns 0 or 1, or <0 to indicate EOF or error
int FileBitStream_read_bit(FileBitStream *input_stream);

int FileBitStream_read_byte(FileBitStream *input_stream);
uint32_t FileBitStream_read_uint32(FileBitStream *input_stream);
int FileBitStream_seek_bit(FileBitStream *input_stream, uint64_t bit_offset);
void FileBitStream_free(FileBitStream *input_stream);