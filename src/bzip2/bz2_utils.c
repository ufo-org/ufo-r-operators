#include "bz2_utils.h"
#include "stdlib.h"

#include "../debug.h"

bz_stream *bz_stream_new() {
    bz_stream *stream = (bz_stream *) malloc(sizeof(bz_stream));
    if (stream == NULL) {
        return NULL;
    }

    stream->next_in = NULL;
    stream->avail_in = 0;
    stream->total_in_lo32 = 0;
    stream->total_in_hi32 = 0;

    stream->next_out = NULL;
    stream->avail_out = 0;
    stream->total_out_lo32 = 0;
    stream->total_out_hi32 = 0;

    stream->state = NULL; 

    stream->bzalloc = NULL;
    stream->bzfree = NULL;
    stream->opaque = NULL;

    return stream;
}

bz_stream *bz_stream_init() {
    bz_stream *stream = bz_stream_new();
    if (stream == NULL) {
        UFO_LOG("cannot allocate a bzip2 stream\n");  
        return NULL;
    }

    int result = BZ2_bzDecompressInit(stream, /*verbosity*/ 0, /*small*/ 0);
    if (result != BZ_OK) {        
        UFO_LOG("cannot initialize a bzip2 stream\n");  
        free(stream);
        return NULL;
    }

    return stream;
}

int bz_stream_read_from_file(bz_stream *stream, FILE* input_file,
                             size_t input_buffer_size, char *input_buffer) {
    
    int read_characters = fread(input_buffer, sizeof(char), input_buffer_size, input_file);
    if (ferror(input_file)) { 
        UFO_LOG("cannot read from file.\n");
        return -1; 
    };        

    // Point the bzip decompressor at the newly read data.
    stream->avail_in = read_characters;
    stream->next_in = input_buffer;

    //REprintf("Read %i bytes from file: %s.\n", read_characters, stream->next_in);
    return read_characters;  
}

