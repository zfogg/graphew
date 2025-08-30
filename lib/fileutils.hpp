#pragma once

#include <stddef.h>

char* read_file_raw(const char* filename, size_t* size);
char* decompress_zlib_data(const char* compressed_data, size_t compressed_size, size_t* decompressed_size);
char* read_compressed_file(const char* filename, size_t* size);