#include "block.h"
#include "blocks.h"
#include "bitstream.h"
#include "bitbuffer.h"
#include "bz2_utils.h"

// #include <stdio.h>
// #include <stdint.h>
// #include <stdbool.h>
// #include <stdlib.h>
// #include <string.h>
#include <errno.h>
#include <assert.h>

#include "../debug.h"

// These are the sizes of magic byte sequences in BZip files. All in bytes.
const int stream_magic_size = 4;
const int block_magic_size = 6;
const int footer_magic_size = 6;

// These are the header and endmark values expected to demarkate blocks.
const unsigned char stream_magic[] = { 0x42, 0x5a, 0x68, 0x30 };
const unsigned char block_magic[]  = { 0x31, 0x41, 0x59, 0x26, 0x53, 0x59 };
const unsigned char footer_magic[] = { 0x17, 0x72, 0x45, 0x38, 0x50, 0x90 };

void Block_free(Block *block) {
    UFO_LOG("Free %p\n", block);
    UFO_LOG("Free %p\n", block->buffer);
    free(block->buffer);
    free(block);
}

Block *Block_from(Blocks *boundaries, size_t index) {

    if (boundaries->blocks <= index) {
        UFO_REPORT("No block at index %li.\n", index);
        return NULL;
    }

    // Retrieve the offsets, for convenience
    const uint64_t block_start_offset_in_bits = boundaries->start_offset[index];
    const uint64_t block_end_offset_in_bits = boundaries->end_offset[index]; // Inclusive
    const uint64_t payload_size_in_bits = block_end_offset_in_bits - block_start_offset_in_bits + 1;
    const uint64_t header_end_offset_in_bits = boundaries->start_offset[0];

    UFO_LOG("Reading block %li from file %s between offsets %li and %li (%lib)\n",
        index, boundaries->path, block_end_offset_in_bits, 
        block_start_offset_in_bits, payload_size_in_bits);

    // StreamHeader    =32b
    //  - HeaderMagic   16b   0x425a            B Z
    //  - Version        8b   0x68              h 
    //  - Level          8b   0x31-0x39         1-9
    // BlockHeader    =105b
    //  - BlockMagic    48b   0x314159265359    π
    //  - BlockCRC      32b                     CRC-32 checksum of uncompressed data
    //  - Randomized     1b   0                 zero (depracated)         
    //  - OrigPtr       24b                     original pointer (BWT stage, internal)
    // StreamFooter    =80b + 0-7b
    //  - FooterMagic   48b   0x177245385090    sqrt π 
    //  - StreamCRC     32b   
    //  - Padding       0-7b    

    // Assume the first boundary is at a byte boundary, otherwise something is very wrong
    assert(boundaries->start_offset[0] % 8 == 0);

    // Calculate size of buffer through the power of arithmetic
    uint64_t buffer_size_in_bits = 
        /* stream header size */ 32 +
        /* block header size  */ 105 +
        /* contents size      */ payload_size_in_bits +
        /* stream footer size */ 80;

    // Account for alignment at the end of stream footer, add 0-7
    uint8_t padding = (8 - (buffer_size_in_bits % 8)) % 8;
    buffer_size_in_bits += padding;

    // Calculate bytes, for convenience
    uint64_t buffer_size_in_bytes = buffer_size_in_bits / 8;


    UFO_LOG("Preparing buffer of size %lib = %ib + %ib + %lib + %ib + %ib\n",
        buffer_size_in_bits, 32, 105, payload_size_in_bits, 80, padding);

    // Alloc the buffre for the chunk
    unsigned char *buffer = (unsigned char *) calloc(buffer_size_in_bytes, sizeof(unsigned char));
    memset(buffer, 0, buffer_size_in_bytes);

    // Open file for reading the remainder
    FileBitStream *input_stream = FileBitStream_new(boundaries->path);
    if (input_stream == NULL) {
        UFO_REPORT("cannot open bit stream for %s\n", boundaries->path);
        // TODO free what needs freeing
        return NULL;
    }

    // Read in everything before the first block boundary
    size_t read_bytes = 0;
    while (input_stream->read_bits < header_end_offset_in_bits) {
        int byte = FileBitStream_read_byte(input_stream);
        if (byte < 0) {
            UFO_REPORT("cannot read byte from bit stream\n");
            FileBitStream_free(input_stream);
            // TODO free what needs freeing
            return NULL;
        }
        buffer[read_bytes] = byte;
        read_bytes++;
    }

    // Seek to the block boundary
    int seek_result = FileBitStream_seek_bit(input_stream, block_start_offset_in_bits);
    if (seek_result < 0) {
        UFO_REPORT("cannot seek to %lib offset\n", block_start_offset_in_bits);
        return NULL;
    }

    // Read 32-bit CRC, which is just behind the boundary
    uint32_t block_crc = FileBitStream_read_uint32(input_stream);
    UFO_LOG("Block CRC %08x.\n", block_crc);
    if (block_crc < 0) {
        UFO_REPORT("Cannot read uint32 from bit stream.\n");
        FileBitStream_free(input_stream);
        // TODO free what needs freeing
        return NULL;
    }

    for (int i = sizeof(uint32_t) - 1; i >= 0; i--) {
        buffer[read_bytes++] = 0xff & (block_crc >> (8 * i));
    }
    
    // This is how much we can copy byte-by-byte before getting into trouble at
    // the end of the file. The remainder we have to copy bit-by-bit.
    size_t block_end_byte_aligned_offset_in_bits = (payload_size_in_bits & (~0x07)) + block_start_offset_in_bits;
    size_t remaining_byte_unaligned_bits = (payload_size_in_bits & (0x07));

    UFO_LOG("The block has to read until offset %lib = %lib + %lib. [%lib]\n", 
            block_end_offset_in_bits,
            block_end_byte_aligned_offset_in_bits, 
            remaining_byte_unaligned_bits,
            block_start_offset_in_bits);

    // Read until the last byte-aligned datum of the block
    while (input_stream->read_bits < block_end_byte_aligned_offset_in_bits) {
        int byte = FileBitStream_read_byte(input_stream);
        if (byte < 0) {
            UFO_REPORT("Cannot read byte from bit stream.\n");
            FileBitStream_free(input_stream);
            // TODO free what needs freeing
            return NULL;
        }
        buffer[read_bytes] = byte;
        read_bytes++;
    }

    // Ok, now we write the rest into a buffer to align it.
    BitBuffer *bit_buffer = BitBuffer_new(1 + footer_magic_size + 4); // TODO size
    for (int annoying_unaligned_bit = 0; 
         annoying_unaligned_bit < remaining_byte_unaligned_bits ; // sus
         annoying_unaligned_bit++) {

        // Read a bit from file, yay
        int bit = FileBitStream_read_bit(input_stream);
        if (bit < 0) {
            UFO_REPORT("cannot read bit from bit stream.\n");
            return NULL;
        }

        // Write that bit to buffer, yay
        int result = BitBuffer_append_bit(bit_buffer, (unsigned char) bit);
        if (result < 0) {
            UFO_REPORT("Cannot write bit to bit buffer.\n");
            return NULL;
        }
    }

    // Ok, now we write out the footer to the buffer, to align that.
    for (int footer_magic_byte = 0; 
         footer_magic_byte < footer_magic_size; 
         footer_magic_byte++) {

        int result = BitBuffer_append_byte(bit_buffer, footer_magic[footer_magic_byte]);
        if (result < 0) {
            UFO_REPORT("cannot write magic footer byte to bit buffer\n");
            return NULL;
        }
    }

    // Write out the block CRC value as the stream CRC value, since it's just
    // one block.
    int result = BitBuffer_append_uint32(bit_buffer, block_crc);
    if (result < 0) {
        UFO_REPORT("cannot write CRC byte to bit buffer\n");
        return NULL;
    }  

    // Write the bit buffer into the actual buffer.
    for (int i = 0; i < bit_buffer->max_bytes; i++) {
        buffer[read_bytes + i] = bit_buffer->data[i];
    }
    read_bytes += bit_buffer->max_bytes;

    // Return block data
    Block *block = malloc(sizeof(Block));
    block->buffer = buffer;
    block->size = read_bytes;
    block->max_size = buffer_size_in_bytes;

    // Free what needs freeing
    BitBuffer_free(bit_buffer);

    // Do not release buffer, because that's what we return. Make sure to free
    // it afterwards though.

    return block;
}

int Block_decompress(Block *block, size_t output_buffer_size, char *output_buffer) {

    // Hand-crank the BZip2 decompressor
    bz_stream *stream = bz_stream_init();
    if (stream == NULL) {
        UFO_REPORT("cannot initialize stream\n");  
        return -1;
    }         

    // Set array contents to zero, just in case.
    memset(output_buffer, 0x7c, output_buffer_size);

    // Tell BZip2 where to write decompressed data.
	stream->avail_out = output_buffer_size;
	stream->next_out = output_buffer;

    // Tell BZip2 where to getr input data from.
    stream->avail_in = block->size;
    stream->next_in  = (char *) block->buffer;

    // Do the do.
    int result = BZ2_bzDecompress(stream);

    // LOG("Decompressed bytes (only 40 first): ");
    // for (int i = 0; i < 40; i++) {
    //     LOG_SHORT("%02x ", output_buffer[i]);
    // }
    // LOG_SHORT("\n");

    if (result != BZ_OK && result != BZ_STREAM_END) { 
        UFO_REPORT("Cannot process stream, error no.: %i.\n", result);
        BZ2_bzDecompressEnd(stream);
        return -3;
    };

    if (result == BZ_OK 
        && stream->avail_in == 0 
        && stream->avail_out > 0) {
        BZ2_bzDecompressEnd(stream);
        UFO_REPORT("Cannot process stream, unexpected end of file.\n");        
        return -4; 
    };

    if (result == BZ_STREAM_END) {        
        UFO_LOG("Finshed processing stream.\n");  
        BZ2_bzDecompressEnd(stream); 
        return output_buffer_size - stream->avail_out;
    };

    if (stream->avail_out == 0) {   
        UFO_LOG("Stream ended: no data read.\n");  
        BZ2_bzDecompressEnd(stream);
        return output_buffer_size; 
    };

    // Supposedly unreachable.
    UFO_REPORT("Unreachable isn't.\n");  
    BZ2_bzDecompressEnd(stream);
    return 0;
}