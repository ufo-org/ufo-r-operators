#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "blocks.h"

typedef struct {
    unsigned char   *buffer;
    size_t  size;
    size_t  max_size;
} Block;

Block *Block_from(Blocks *boundaries, size_t index);
void Block_free(Block *block);
int Block_decompress(Block *block, size_t output_buffer_size, char *output_buffer);