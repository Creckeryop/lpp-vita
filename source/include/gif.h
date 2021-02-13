//code is written by creckeryop
#ifndef GIF_LIBRARY_H
#define GIF_LIBRARY_H

#include <stdint.h>
#include <stdio.h>

#define RGBA8(r,g,b,a) ((((a)&0xFF)<<24) | (((b)&0xFF)<<16) | (((g)&0xFF)<<8) | (((r)&0xFF)<<0))

typedef enum GIFStatus {
    GIF_OK                      = 0x0000,
    GIF_ERROR                   = 0x0001,
    GIF_CHUNK_MALLOC_ERROR      = 0x0002,
    GIF_READ_MALLOC_ERROR       = 0x0003,
    GIF_NO_ENOUGH_CHUNK_ERROR   = 0x0004,
    GIF_WRONG_CHUNK_SIZE_ERROR  = 0x0005,
    GIF_GIF_NULL_PTR_ERROR      = 0x0006,
    GIF_HANDLER_NULL_PTR_ERROR  = 0x0007,
    GIF_OPEN_FILE_ERROR         = 0x0008,
    GIF_UNSUPPORTED_FORMAT      = 0x0009,
    GIF_NO_MEMORY_FOR_GCT       = 0x000A,
    GIF_NO_MEMORY_FOR_LCT       = 0x000B,
    GIF_END_FRAMES              = 0x000C,
    GIF_NO_MEMORY               = 0x000D
} GIFStatus;

typedef enum GIFMagic {
    GIFNAN = 0,
    GIF87A = 1,
    GIF89A = 2
} GIFMagic;

struct GIFHandler {
    GIFStatus (*init_f)(void*, void*);
    size_t (*read_f)(void*, size_t, void*);
    GIFStatus (*term_f)(void*);
    size_t chunk_size;
    size_t fd_size;
};

struct GIFChunk {
    size_t size;
    size_t i;
    uint8_t* data;
    uint8_t is_last;
};

struct GIFBuffer {
    size_t size;
    uint8_t* data;
};

struct GIFObject {
    GIFMagic magic;
    size_t width;
    size_t height;
    size_t delay;
    size_t gct_size;
    uint32_t* frame;
    uint32_t* fframe;
    uint32_t* gct;
    int16_t bkg_color;
    int8_t scale;
    struct GIFHandler* handler;
    struct GIFChunk chunk;
    struct GIFBuffer bytes;
    void* descriptor;
};


GIFStatus OpenGIF(struct GIFHandler* handler, struct GIFObject* gif, void* object);
GIFStatus GetFrameGIF(struct GIFObject* gif);
GIFStatus FreeGIF(struct GIFObject* gif);

#endif