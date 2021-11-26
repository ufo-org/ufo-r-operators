#include "ufo_bz2.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#define USE_RINTERNALS
#include <R.h>
#include <Rinternals.h>

#include "../include/ufo_r/src/ufos.h"
#include "helpers.h"
#include "debug.h"

//#include "evil/bad_strings.h"

#include "ufo_bz2.h"
#include "bzip2/blocks.h"

// #include <bzlib.h>

static int32_t BZip2_populate_bytes(void* user_data, uintptr_t start, uintptr_t end, unsigned char* target) {

    Blocks *blocks = (Blocks *) user_data;

    // TODO DEBUG
    for (size_t block = 0; block < blocks->blocks; block ++) {
        REprintf("UFO checking block lengths: block %li : decompressed %li-%li [%li]\n", block,
            blocks->decompressed_start_offset[block], blocks->decompressed_end_offset[block], 
            blocks->decompressed_end_offset[block] - blocks->decompressed_start_offset[block] + 1);
    }

    size_t block_index = 0;
    bool found_start = false;

    // Ignore blocks that do not are not a part of the current segment.
    for (; block_index < blocks->blocks; block_index++) {
        if (start >= blocks->decompressed_start_offset[block_index] && start <= blocks->decompressed_end_offset[block_index]) { 
            found_start = true;
            // TODO DEBUG
            REprintf("UFO will start at block %li: start=%li <= decompressed_offset=%li\n", 
                block_index, start, blocks->decompressed_start_offset[block_index]);
            break;
        }
        LOG("UFO skips block %li: start=%li <= decompressed_offset=%li \n", 
            block_index, start, blocks->decompressed_start_offset[block_index]);
    }

    // The start index is not within any of the blocks?
    if (!found_start) {
        REprintf("Start index %li is not within any of the BZip2 blocks.\n", start); 
        return 1;
    }

    // At this point block index is the index of the block containing the start.
    // We calculate the bytes_to_skip: the offset of the start of the UFO segment with
    // respect to the first value in the Bzip2 block.
    size_t bytes_to_skip = start - (blocks->decompressed_start_offset[block_index]);
    // TODO DEBUG
    REprintf("UFO will skip %lu bytes of block %lu\n", bytes_to_skip, block_index);

    // The index in the target buffer.
    // size_t target_index = 0;

    // Temp buffers, because I don't know how to do it without it.
    size_t decompressed_buffer_size = DECOMPRESSED_BUFFER_SIZE; // 1MB
    char *decompressed_buffer = (char *) calloc(decompressed_buffer_size, sizeof(char));
    // memset(decompressed_buffer, 0x5c, decompressed_buffer_size); // FIXME remove

    // Check if we filled all the requested bytes from all the necessary BZip blocks.
    bool found_end = false;

    // The offset in target at which to paste the next decompressed BZip block.
    size_t offset_in_target = 0;

    // The number of bytes we still have to generate.
    size_t left_to_fill_in_target = end - start;

    // Decompress blocks, until required number of decompressed blocks produce
    // the prerequisite number of bytes of data.
    for (; block_index < blocks->blocks; block_index++) {

        // TODO DEBUG
        REprintf("UFO loads and decompresses block %li.\n", block_index);

        // Retrive and decompress the block.
        Block *block = Block_from(blocks, block_index);
        int decompressed_buffer_occupancy = Block_decompress(block, decompressed_buffer_size, decompressed_buffer);
        if (decompressed_buffer_occupancy <= 0) {
            // TOOD DEBUG
            REprintf("UFO failed to decompress BZip.\n");
            return -1;
        } else {
            // TODO DEBUG
            REprintf("UFO retrieved %i elements by decompressing block %li.\n", 
                decompressed_buffer_occupancy, block_index);
        }
        size_t elements_to_copy = decompressed_buffer_occupancy - bytes_to_skip;
        // TODO DEBUG
        REprintf("UFO can retrieve %lu = %i - %li elements from BZip block "
            "and needs to grab %lu elements to fill the target location.\n", 
            elements_to_copy, decompressed_buffer_occupancy, bytes_to_skip, left_to_fill_in_target);
        if (elements_to_copy > left_to_fill_in_target) {            
            elements_to_copy = left_to_fill_in_target;
        }        
        // TODO DEBUG
        REprintf("UFO will grab %lu elements from BZip decompressed block to fill the target location.\n", 
            elements_to_copy);
        assert(decompressed_buffer_occupancy >= bytes_to_skip);

        // TODO DEBUG
        REprintf("UFO copies %lu elements from decompressed block %lu to target area %p = %p + %lu\n", 
            elements_to_copy, block_index, decompressed_buffer + bytes_to_skip, 
            decompressed_buffer, bytes_to_skip);

        // Copy the contents of the block to the target area. I can't do this
        // without this intermediate buffer, because the first block might need
        // to discard some number of bytes from the front. 
        // 
        // Also the last one has to disregards some number of bytes from the
        // back, but the block won't produce anything unless it's given room to
        // write out the whole block. <-- TODO: this needs verification        
        memcpy(/* destination */ target + offset_in_target, 
               /* source      */ decompressed_buffer + bytes_to_skip, 
               /* elements    */ elements_to_copy); // TODO remove elements to make sure we don't load more than `end`

        // Start filling the target in the next iteration from the palce we
        // finished here.
        offset_in_target += elements_to_copy;

        // We copied these many elements already.
        left_to_fill_in_target -= elements_to_copy;     

        // Only the first block has bytes_to_skip != 0.
        bytes_to_skip = 0;   

        // Cleanup.
        Block_free(block);        

        if ((end - 1) >= blocks->decompressed_start_offset[block_index] 
            && (end - 1) <= blocks->decompressed_end_offset[block_index]) { 
            found_end = true;
            break;
        }
    }

    // Cleanup
    free(decompressed_buffer);

    // TODO check if found end
    if (!found_end) {
        REprintf("did not find a block containing the end of "
                "the segment range [%li-%li].\n", start, end);
        return 1;
    }

    return 0;
}

/*ufo_vector_type_t result_type, */
SEXP ufo_bz2_raw(SEXP/*STRSXPXP*/ filename, SEXP/*LGLSXP*/ read_only, SEXP/*INTSXP*/ min_load_count) {

    // Read the arguements into practical types (with checks).
    bool read_only_value = __extract_boolean_or_die(read_only);
    int min_load_count_value = __extract_int_or_die(min_load_count);
    const char *path = __extract_path_or_die(filename);

    // Pre-scan the file
    Blocks *blocks = Blocks_new(filename);

        // Check for bad blocks
    if (blocks->bad_blocks > 0) {
        Blocks_free(blocks);
        Rf_error("UFO some blocks could not be read. Quitting.\n");
        return R_NilValue;
    }

    // Create a source struct for UFOs.
    ufo_source_t* source = (ufo_source_t*) malloc(sizeof(ufo_source_t));

    // Element size and count metadata
    source->vector_type = UFO_RAW;
    source->element_size = __get_element_size(result_type);
    source->vector_size = blocks->decompressed_size;

    // Behavior specification
    source->data = (void*) blocks;
    source->destructor_function = BZip2_ufo_free; //&destroy_data;
    source->population_function = BZip2_populate;
    
    // Chunk-related parameters
    source->read_only = read_only_value;
    source->min_load_count = __select_min_load_count(min_load_count_value, source->element_size);

    // Unused.
    source->dimensions = NULL;
    source->dimensions_length = 0;

    ufo_new_t ufo_new = (ufo_new_t) R_GetCCallable("ufos", "ufo_new");
    return ufo_new(source);
}

void BZip2_ufo_free(void* data) {
    // Free the blocks struct
    Blocks *blocks = (Blocks *) data;
    Blocks_free(blocks);

    // Everything else is handled by UFO-R.
}