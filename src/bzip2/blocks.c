#include "blocks.h"
#include "block.h"
#include "bitstream.h"
#include "shift.h"

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

    boundaries->path = input_file_path;
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
    // Nothing to free, 
    free(blocks);
}

Blocks *Blocks_new(const char *filename) {
    // Parse the file.
    Blocks *blocks = Blocks_parse(filename);

    // Create the structures for outputting the decompressed data into
    // (temporarily).
    size_t output_buffer_size = 1024 * 1024 * 1024; // 1MB
    char *output_buffer = (char *) calloc(output_buffer_size, sizeof(char));
    
    // Unfortunatelly, we need to scan the entire file to figure out where the blocks go when decompressed.
    size_t end_of_last_decompressed_block = 0;
    for (size_t i = 0; i < blocks->blocks; i++) {

        // Extract a single compressed block.
        // __block = i;
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