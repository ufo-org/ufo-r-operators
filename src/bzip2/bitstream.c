#include "bitstream.h"

#include <stdlib.h>
#include "../debug.h"

FileBitStream *FileBitStream_new(const char *input_file_path) {
    // Allocate bit stream for reading
    FileBitStream *input_stream = (FileBitStream *) malloc(sizeof(FileBitStream));
    if (input_stream == NULL) {
        UFO_REPORT("Cannot allocate FileBitStream object.\n");  
        return NULL;
    }

    // Initially the buffer is empty
    input_stream->buffer = 0;
    input_stream->buffer_live = 0;
    input_stream->read_bits = 0;

    // Initally the shift register is empty
    input_stream->shift_register.senior = 0;
    input_stream->shift_register.junior = 0;

    // Open the input bzip2 file
    FILE* input_file = fopen(input_file_path, "rb");
    if (input_file == NULL) {
        UFO_REPORT("Cannot read file at %s.\n", input_file_path);
        free(input_stream);
        return NULL;
    }

    // Update file info
    input_stream->path = input_file_path; // FIXME 0-terminate
    input_stream->handle = input_file;   
    return input_stream;
}

// Returns 0 or 1, or <0 to indicate EOF or error
int FileBitStream_read_bit(FileBitStream *input_stream) {

    // always score a hit
    input_stream->read_bits++;

    // If there is a bit in the buffer, return that. Append the hot bit into the
    // shift register buffer.
    if (input_stream->buffer_live > 0) {
        input_stream->buffer_live--;
        int bit = (((input_stream->buffer) >> (input_stream->buffer_live)) & 0x1);
        ShiftRegister_append(&input_stream->shift_register, bit);
        return bit;
    }

    // Read a byte from the input file
    int byte = getc(input_stream->handle);

    // Detect end of file or error
    if (byte == EOF) {
        // Detect error
        if (ferror(input_stream->handle)) {
            perror("ERROR");
            UFO_REPORT("Cannot read bit from file at %s.\n", 
                    input_stream->path);         
            return -2;
        }

        // Detect end of file
        return -1;
    }

    // Take the first bit form the read byte and return that. Stick the rest
    // into the buffer. Append the hot bit into the shift register buffer.
    input_stream->buffer_live = 7;
    input_stream->buffer = byte;
    int bit = (((input_stream->buffer) >> 7) & 0x1);
    ShiftRegister_append(&input_stream->shift_register, bit);
    return bit;
}

int FileBitStream_read_byte(FileBitStream *input_stream) {
    for (int bit = 0; bit < 8; bit++) {
        int result = FileBitStream_read_bit(input_stream);
        if (result < 0) {
            return result;
        }
    }
    return ShiftRegister_junior_byte(&input_stream->shift_register);
}

uint32_t FileBitStream_read_uint32(FileBitStream *input_stream) {
    for (int bit = 0; bit < 32; bit++) {
        int result = FileBitStream_read_bit(input_stream);
        if (result < 0) {
            return result;
        }
    }
    return input_stream->shift_register.junior;
}

int FileBitStream_seek_bit(FileBitStream *input_stream, uint64_t bit_offset) {

    // Don't feel like solving the case for < 64, we never use it, since the
    // first block will start around 80b.
    if (bit_offset < (sizeof(uint32_t) * 2)) {
        UFO_REPORT("Seeking to bit offset < 64b is not allowed.\n");
    }

    // Maff
    uint64_t byte_offset = bit_offset / 8;
    uint64_t byte_offset_without_buffer = byte_offset - (sizeof(uint32_t) * 2); // Computron will maff
    int remainder_bit_offset = bit_offset % 8;

    // Reset the buffer
    input_stream->buffer = 0;
    input_stream->buffer_live = 0;

    // Move to byte offset
    int seek_result = fseek(input_stream->handle, byte_offset_without_buffer, SEEK_SET);
    if (seek_result < -1) {
        perror("ERROR");
        UFO_REPORT("Seek failed in %s\n", input_stream->path);
        return seek_result;
    }

    // Repopulate the shift register. This should read 64b, and we should be at
    // `byte offset` afterwards.
    size_t read_bytes = fread(&input_stream->shift_register.senior, sizeof(uint32_t), 1, input_stream->handle);
    if (read_bytes != 1) {
        UFO_REPORT("Error populating senior part of shift buffer " 
                "(read %li bytes instead of the expected %i).\n", 
                read_bytes, 1);
    }
    read_bytes = fread(&input_stream->shift_register.junior, sizeof(uint32_t), 1, input_stream->handle);
    if (read_bytes != 1) {
        UFO_REPORT("Error populating junior part of shift buffer " 
                "(read %li bytes instead of the expected %i).\n", 
                read_bytes, 1);
    }

    // Seeking counts as reading the bits, so we account for all the bits. We
    // oly ever seek form the beginning of the file, so this is easy.
    input_stream->read_bits = byte_offset * 8;

    // Move to bit offset
    for (int extra_bit = 0; extra_bit < remainder_bit_offset; extra_bit++) {
        int bit = FileBitStream_read_bit(input_stream);
        if (bit == -1) {
            UFO_REPORT("Seek error, unexpected EOF at %li.", 
                    byte_offset * 8 + extra_bit);
            return -1;
        }
        if (bit == -2) {
            UFO_REPORT("Seek error.");
            return -2;
        }        
    }    

    // Victory is ours.
    return 0;
}

void FileBitStream_free(FileBitStream *input_stream) {   
    // Close file associated with stream
    if (fclose(input_stream->handle) == EOF) {
        UFO_WARN("Failed to close file at %s.\n", input_stream->path); 
    }

    // Free memory
    free(input_stream);
}