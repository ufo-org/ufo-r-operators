
#pragma once

#include <bzlib.h>

bz_stream *bz_stream_new();

bz_stream *bz_stream_init();

int bz_stream_read_from_file(bz_stream *stream, FILE* input_file,
                             size_t input_buffer_size, char *input_buffer);