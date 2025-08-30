#include "fileutils.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

char* read_file_raw(const char* filename, size_t* size) {
    if (!filename || !size) return NULL;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (length < 0) {
        fclose(file);
        return NULL;
    }
    
    char* data = static_cast<char*>(malloc(length + 1));
    if (!data) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(data, 1, length, file);
    fclose(file);
    
    if (read_size != (size_t)length) {
        free(data);
        return NULL;
    }
    
    data[length] = '\0';
    *size = length;
    return data;
}

char* decompress_zlib_data(const char* compressed_data, size_t compressed_size, size_t* decompressed_size) {
    if (!compressed_data || !decompressed_size || compressed_size == 0) return NULL;
    
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    
    if (inflateInit(&strm) != Z_OK) {
        return NULL;
    }
    
    size_t buffer_size = compressed_size * 4;
    char* output_buffer = static_cast<char*>(malloc(buffer_size));
    if (!output_buffer) {
        inflateEnd(&strm);
        return NULL;
    }
    
    strm.next_in = (Bytef*)compressed_data;
    strm.avail_in = compressed_size;
    strm.next_out = (Bytef*)output_buffer;
    strm.avail_out = buffer_size;
    
    int ret;
    size_t total_out = 0;
    
    do {
        ret = inflate(&strm, Z_NO_FLUSH);
        
        if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            free(output_buffer);
            inflateEnd(&strm);
            return NULL;
        }
        
        size_t have = buffer_size - strm.avail_out;
        total_out += have;
        
        if (ret != Z_STREAM_END && strm.avail_out == 0) {
            buffer_size *= 2;
            char* new_buffer = static_cast<char*>(realloc(output_buffer, buffer_size));
            if (!new_buffer) {
                free(output_buffer);
                inflateEnd(&strm);
                return NULL;
            }
            output_buffer = new_buffer;
            strm.next_out = (Bytef*)(output_buffer + total_out);
            strm.avail_out = buffer_size - total_out;
        }
        
    } while (ret != Z_STREAM_END);
    
    inflateEnd(&strm);
    
    output_buffer[total_out] = '\0';
    *decompressed_size = total_out;
    
    return output_buffer;
}

char* read_compressed_file(const char* filename, size_t* size) {
    if (!filename || !size) return NULL;
    
    size_t compressed_size;
    char* compressed_data = read_file_raw(filename, &compressed_size);
    if (!compressed_data) return NULL;
    
    char* decompressed = decompress_zlib_data(compressed_data, compressed_size, size);
    free(compressed_data);
    
    return decompressed;
}