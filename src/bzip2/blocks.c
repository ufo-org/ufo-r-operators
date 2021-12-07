#include "blocks.h"
#include "block.h"
#include "bitstream.h"
#include "shift.h"

#include "../safety_first.h"
#include "../debug.h"

Blocks *Blocks_parse(const char *input_file_path) {

    // Open file for reading.
    FileBitStream *input_stream = FileBitStream_new(input_file_path);
    if (input_stream == NULL) {
        UFO_LOG("Cannot open file %s.\n", input_file_path);  
        return NULL;
    }

    size_t blocks = 0;
    uint64_t start_offset[MAX_BLOCKS];
    uint64_t end_offset[MAX_BLOCKS];

    // Initialize the counters.
    Blocks *boundaries = (Blocks *) malloc(sizeof(Blocks));    
    if (NULL == boundaries) {
        UFO_REPORT("Cannot allocate a struct for recording block boundaries.\n");  
        return NULL;
    }

    boundaries->path = strdup(input_file_path);
    boundaries->blocks = 0;
    boundaries->bad_blocks = 0;

    // Shifting buffer consisting of two parts: senior (most significant) and
    // junior (least significant).
    //ShiftRegister shift_register = {0, 0};
    
    while (true) {
        int bit = FileBitStream_read_bit(input_stream);

        // In case of errors or encountering end of file. We recover, although
        // we probably shouldn't. I don't want to mess with the function too
        // much.
        if (bit < 0) {
            // If we're inside a block (not at the beginning or within an end
            // marker) we UFO_REPORT an error and stop processing the block.
            if (input_stream->read_bits >= start_offset[blocks] && 
               (input_stream->read_bits - start_offset[blocks]) >= 40) {
                end_offset[blocks] = input_stream->read_bits - 1;
                if (blocks > 0) {
                    UFO_LOG("Block %li runs from %li to %li (incomplete)\n",
                            blocks, 
                            start_offset[blocks], 
                            end_offset[blocks]);

                    boundaries->bad_blocks++;
                }               
            } else {
                // Otherwise, I guess this is just EOF?
                blocks--;
            }

            // Proceed to next block.
            break;
        }

        // Detect block header or block end mark
        if (ShiftRegister_equal_with_senior_mask(&input_stream->shift_register, &block_header_template, 0x0000ffff)
            || ShiftRegister_equal_with_senior_mask(&input_stream->shift_register, &block_endmark_template, 0x0000ffff)) {

            // If we found a bounary, the end of the preceding block is 49 bytes
            // ago (or zero if it's the first block)
            end_offset[blocks] = 
                (input_stream->read_bits > 49) ? input_stream->read_bits - 49 : 0;

            // We finished reading a whole block
            if (blocks > 0 
                && (end_offset[blocks] 
                - start_offset[blocks]) >= 130) {

                UFO_LOG("Block %li runs from %li to %li\n",
                    boundaries->blocks + 1, 
                    start_offset[blocks], 
                    end_offset[blocks]);

                // Store the offsets
                boundaries->start_offset[boundaries->blocks] = 
                    start_offset[blocks];
                boundaries->end_offset[boundaries->blocks] = 
                    end_offset[blocks];

                // Increment number of complete blocks so far
                boundaries->blocks++;

                //start_offset[blocks] = input_stream->read_bits;
            }

            // Too many blocks
            if (blocks >= MAX_BLOCKS) {
                UFO_REPORT("analyzer can handle up to %i blocks, "
                                "but more blocks were found in file %s\n",
                                MAX_BLOCKS, input_stream->path);
                free(boundaries);
                return NULL;
            }

            // Increment number of blocks encountered so far and record the
            // start offset of next encountered block
            blocks++;
            start_offset[blocks] = input_stream->read_bits;
        }
    }

    // Clean up and exit
    FileBitStream_free(input_stream);        
    return boundaries;
}

void Blocks_free(Blocks *blocks) {
    free((void *) blocks->path); // because allocated by strdup
    free(blocks);
}

Blocks *Blocks_new(const char *filename, size_t buffer_size) {
    // Parse the file.
    Blocks *blocks = Blocks_parse(filename);

    // Create the structures for outputting the decompressed data into
    // (temporarily).
    blocks->buffer_size = buffer_size;
    size_t output_buffer_size = 1024 * 1024 * 1024; // 1MB
    char *output_buffer = (char *) calloc(output_buffer_size, sizeof(char));
    
    // Unfortunatelly, we need to scan the entire file to figure out where the blocks go when decompressed.
    size_t end_of_last_decompressed_block = 0;
    for (size_t i = 0; i < blocks->blocks; i++) {

        // Extract a single compressed block.
        Block *block = Block_from(blocks, i);        
        int output_buffer_occupancy = Block_decompress(block, output_buffer_size, output_buffer);
        UFO_LOG("Finished decompressing block %li, found %i elements\n", i, output_buffer_occupancy);   

        // Cleanup: we don't actually need the block for anything, except size.
        Block_free(block);

        if (output_buffer_occupancy < 0) {
            UFO_REPORT("Failed to decompress block %li, stopping\n", i);
            break;
        }

        // Calculate start and end of this block when it is decompressed.
        // TODO we could defer some of this.
        blocks->decompressed_start_offset[i] = end_of_last_decompressed_block + (0 == i ? 0 : 1);        
        blocks->decompressed_end_offset[i] = blocks->decompressed_start_offset[i] + output_buffer_occupancy;
        end_of_last_decompressed_block = blocks->decompressed_end_offset[i];        
    }

    // Cleanup
    free(output_buffer);

    // Add one, because end is inclusive.
    blocks->decompressed_size = end_of_last_decompressed_block + 1;
    return blocks;
}

int32_t Blocks_read(Blocks *blocks, uintptr_t start /*byte index*/, uintptr_t end /*byte index*/, unsigned char* target) {

    for (size_t block = 0; block < blocks->blocks; block ++) {
        UFO_LOG("UFO checking block lengths: block %li : decompressed %li-%li [%li]\n", block,
            blocks->decompressed_start_offset[block], blocks->decompressed_end_offset[block], 
            blocks->decompressed_end_offset[block] - blocks->decompressed_start_offset[block] + 1);
    }

    size_t block_index = 0;
    bool found_start = false;

    // Ignore blocks that do not are not a part of the current segment.
    for (; block_index < blocks->blocks; block_index++) {
        if (start >= blocks->decompressed_start_offset[block_index] && start <= blocks->decompressed_end_offset[block_index]) { 
            found_start = true;
            UFO_LOG("UFO will start at block %li: start=%li <= decompressed_offset=%li\n", 
                block_index, start, blocks->decompressed_start_offset[block_index]);
            break;
        }
       UFO_LOG("UFO skips block %li: start=%li <= decompressed_offset=%li \n", 
            block_index, start, blocks->decompressed_start_offset[block_index]);
    }

    // The start index is not within any of the blocks?
    if (!found_start) {
        UFO_LOG("Start index %li is not within any of the BZip2 blocks.\n", start); 
        return 1;
    }

    // At this point block index is the index of the block containing the start.
    // We calculate the bytes_to_skip: the offset of the start of the UFO segment with
    // respect to the first value in the Bzip2 block.
    size_t bytes_to_skip = start - (blocks->decompressed_start_offset[block_index]);
    UFO_LOG("UFO will skip %lu bytes of block %lu\n", bytes_to_skip, block_index);

    // The index in the target buffer.
    // size_t target_index = 0;

    // Temp buffers, because I don't know how to do it without it.
    size_t decompressed_buffer_size = blocks->buffer_size; // 1MB
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

        UFO_LOG("UFO loads and decompresses block %li.\n", block_index);

        // Retrive and decompress the block.
        Block *block = Block_from(blocks, block_index);
        int decompressed_buffer_occupancy = Block_decompress(block, decompressed_buffer_size, decompressed_buffer);
        if (decompressed_buffer_occupancy <= 0) {
            UFO_REPORT("UFO failed to decompress BZip.\n");
            return -1;
        } else {
            UFO_LOG("UFO retrieved %i elements by decompressing block %li.\n", 
                decompressed_buffer_occupancy, block_index);
        }
        size_t elements_to_copy = decompressed_buffer_occupancy - bytes_to_skip;
        UFO_LOG("UFO can retrieve %lu = %i - %li elements from BZip block "
            "and needs to grab %lu elements to fill the target location.\n", 
            elements_to_copy, decompressed_buffer_occupancy, bytes_to_skip, left_to_fill_in_target);
        if (elements_to_copy > left_to_fill_in_target) {            
            elements_to_copy = left_to_fill_in_target;
        }        
        UFO_LOG("UFO will grab %lu elements from BZip decompressed block to fill the target location.\n", 
            elements_to_copy);
        make_sure(decompressed_buffer_occupancy >= bytes_to_skip, "More bytes to skip, than bytes in buffer");

        UFO_LOG("UFO copies %lu elements from decompressed block %lu to target area %p = %p + %lu\n", 
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
        UFO_REPORT("did not find a block containing the end of "
                "the segment range [%li-%li].\n", start, end);
        return 1;
    }

    return 0;
}