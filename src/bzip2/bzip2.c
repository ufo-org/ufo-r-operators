#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include "bzip2/blocks.h"

#include <bzlib.h>

// The bzip code was adapted from bzip2recovery.

// // Sizes of temporary buffers used to chunk and decompress BZip data.
// #define BUFFER_SIZE 1024
// #define DECOMPRESSED_BUFFER_SIZE 1024 * 1024 * 1024 // 900kB + some room just in case

// // This is the maximum number of blocks that we expect in a file. We cannot
// // process more than this number. 50K blocks represents 40GB+ of uncompressed
// // data. 
// #define MAX_BLOCKS 50000 // TODO: grow the vector instead?

// // These are the sizes of magic byte sequences in BZip files. All in bytes.
// const int stream_magic_size = 4;
// const int block_magic_size = 6;
// const int footer_magic_size = 6;

// // These are the header and endmark values expected to demarkate blocks.
// const unsigned char stream_magic[] = { 0x42, 0x5a, 0x68, 0x30 };
// const unsigned char block_magic[]  = { 0x31, 0x41, 0x59, 0x26, 0x53, 0x59 };
// const unsigned char footer_magic[] = { 0x17, 0x72, 0x45, 0x38, 0x50, 0x90 };

