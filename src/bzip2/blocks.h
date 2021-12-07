#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// This is the maximum number of blocks that we expect in a file. We cannot
// process more than this number. 50K blocks represents 40GB+ of uncompressed
// data. 
#define MAX_BLOCKS 50000 // TODO: grow the vector instead? or parameterize

// A structure representing blocks in a BZIP2 file. Records offset of beginning
// and end of each block.
typedef struct {
    const char *path;
    size_t blocks;
    uint64_t start_offset[MAX_BLOCKS];
    uint64_t end_offset[MAX_BLOCKS];
    uint64_t decompressed_start_offset[MAX_BLOCKS];    
    uint64_t decompressed_end_offset[MAX_BLOCKS];   
    size_t bad_blocks;
    size_t decompressed_size;
    size_t buffer_size;
} Blocks;

Blocks *Blocks_parse(const char *input_file_path);
Blocks *Blocks_new(const char *filename, size_t buffer_size);
void Blocks_free(Blocks *blocks);
int32_t Blocks_read(Blocks *blocks, uintptr_t start /*byte index*/, uintptr_t end /*byte index*/, unsigned char* target);

