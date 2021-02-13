//code is written by creckeryop
#include "gif.h"

#include <stdlib.h>
#include <string.h>
#include <vita2d.h>

#define true 1
#define false 0

struct command {
    uint16_t* codes;
    size_t size;
};

GIFStatus read_next_chunk(struct GIFObject* gif) {
    if (gif->chunk.data != NULL) {
        free(gif->chunk.data);
        gif->chunk.data = NULL;
    }
    gif->chunk.i = 0;
    gif->chunk.size = 0;
    gif->chunk.data = (uint8_t*)malloc(sizeof(uint8_t) * gif->handler->chunk_size);
    if (gif->chunk.data == NULL) {
        return GIF_CHUNK_MALLOC_ERROR;
    }
    gif->chunk.size = gif->handler->read_f(gif->descriptor, gif->handler->chunk_size, gif->chunk.data);
    gif->chunk.is_last = gif->chunk.size != gif->handler->chunk_size;
    return GIF_OK;
}

GIFStatus read_next_bytes(struct GIFObject* gif, size_t size) {
    if (gif->bytes.data != NULL) {
        free(gif->bytes.data);
        gif->bytes.data = NULL;
    }
    gif->bytes.size = 0;
    gif->bytes.data = (uint8_t*)malloc(sizeof(uint8_t) * size);
    if (gif->bytes.data == NULL) {
        return GIF_READ_MALLOC_ERROR;
    }
    gif->bytes.size = size;
    if (gif->chunk.data == NULL) {
        GIFStatus gif_err = read_next_chunk(gif);
        if (gif_err) {
            return gif_err;
        }
    }
    int k = 0;
    while (k < size) {
        if (gif->chunk.i + (size - k) > gif->chunk.size) {
            memcpy(gif->bytes.data + k, gif->chunk.data + gif->chunk.i, gif->chunk.size - gif->chunk.i);
            k += gif->chunk.size - gif->chunk.i;
            if (gif->chunk.is_last) {
                if (gif->chunk.data != NULL) {
                    free(gif->chunk.data);
                    gif->chunk.data = NULL;
                    gif->chunk.size = 0;
                    gif->chunk.i = 0;
                }
                return GIF_NO_ENOUGH_CHUNK_ERROR;
            } else {
                GIFStatus gif_err = read_next_chunk(gif);
                if (gif_err) {
                    return gif_err;
                }
            }
        } else {
            memcpy(gif->bytes.data + k, gif->chunk.data + gif->chunk.i, size - k);
            gif->chunk.i += size - k;
            break;
        }
    }
    return GIF_OK;
}

void clear_mem(struct GIFObject* gif) {
    if (gif->descriptor != NULL) {
        gif->handler->term_f(gif->descriptor);
        free(gif->descriptor);
        gif->descriptor = NULL;
    }
    if (gif->chunk.data != NULL) {
        free(gif->chunk.data);
        gif->chunk.i = 0;
        gif->chunk.size = 0;
        gif->chunk.data = NULL;
        gif->chunk.is_last = false;
    }
    if (gif->bytes.data != NULL) {
        free(gif->bytes.data);
        gif->bytes.size = 0;
        gif->bytes.data = NULL;
    }
    if (gif->frame != NULL) {
        free(gif->frame);
        gif->frame = NULL;
    }
    if (gif->fframe != NULL) {
        free(gif->fframe);
        gif->fframe = NULL;
    }
    if (gif->gct != NULL) {
        free(gif->gct);
        gif->gct_size = 0;
        gif->gct = NULL;
    }
}

GIFStatus OpenGIF(struct GIFHandler* handler, struct GIFObject* gif, void* object) {
    if (gif == NULL) {
        return GIF_GIF_NULL_PTR_ERROR;
    }
    if (handler == NULL) {
        return GIF_HANDLER_NULL_PTR_ERROR;
    }
    gif->handler = handler;
    gif->chunk.i = 0;
    gif->chunk.size = 0;
    gif->chunk.data = NULL;
    gif->chunk.is_last = false;
    gif->bytes.size = 0;
    gif->bytes.data = NULL;
    gif->frame = NULL;
    gif->fframe = NULL;
    gif->gct = NULL;
    gif->gct_size = 0;
    if (gif->handler->chunk_size <= 0) {
        return GIF_WRONG_CHUNK_SIZE_ERROR;
    }
    gif->descriptor = malloc(gif->handler->fd_size);
    if (handler->init_f(object, gif->descriptor)) {
        free(gif->descriptor);
        gif->descriptor = NULL;
        return GIF_OPEN_FILE_ERROR;
    }
    GIFStatus gif_err = read_next_bytes(gif, 13);
    if (gif_err) {
        gif->handler->term_f(gif->descriptor);
        free(gif->descriptor);
        gif->descriptor = NULL;
        return gif_err;
    }
    if (!strncmp((char*)gif->bytes.data, "GIF87a", 6)) {
        gif->magic = GIF87A;
    } else if (!strncmp((char*)gif->bytes.data, "GIF89a", 6)) {
        gif->magic = GIF89A;
    } else {
        clear_mem(gif);
        return GIF_UNSUPPORTED_FORMAT;
    }
    gif->width = gif->bytes.data[6] | (gif->bytes.data[7] << 8);
    gif->height = gif->bytes.data[8] | (gif->bytes.data[9] << 8);
    uint8_t gct_flag = (gif->bytes.data[10] >> 7) & 0b0001;
    uint8_t color_resolution = (gif->bytes.data[10] >> 4) & 0b0111;
    uint8_t sort_flag = (gif->bytes.data[10] >> 3) & 0b0001;
    gif->gct_size = 1 << ((gif->bytes.data[10] & 0b0111) + 1);
    gif->bkg_color = gif->bytes.data[11];
    uint8_t pixel_aspect_ratio = gif->bytes.data[12];
    if (gct_flag) {
        if (gif->gct_size != 0) {
            gif->gct = (uint32_t*)malloc(gif->gct_size * sizeof(uint32_t));
            if (gif->gct == NULL) {
                return GIF_NO_MEMORY_FOR_GCT;
            }
            GIFStatus gif_err = read_next_bytes(gif, gif->gct_size * 3);
            if (gif_err) {
                clear_mem(gif);
                return gif_err;
            }
            for (size_t i = 0, j = 0; i < gif->gct_size; ++i, j += 3) {
                gif->gct[i] = RGBA8(gif->bytes.data[j], gif->bytes.data[j + 1], gif->bytes.data[j + 2], 0xFF);
            }
        }
    } else {
        gif->gct_size = 0;
    }
    return GIF_OK;
}

GIFStatus GetFrameGIF(struct GIFObject* gif) {
    if (gif == NULL) {
        return GIF_GIF_NULL_PTR_ERROR;
    }
    if (gif->frame == NULL) {
        gif->frame = (uint32_t*)malloc(sizeof(uint32_t) * gif->width * gif->height);
    }
    uint32_t* colors = gif->gct;
    size_t colors_size = gif->gct_size;
    size_t a_width = gif->width;
    size_t a_height = gif->height;
    int transparent_id = -1;
    int start_x = 0;
    int start_y = 0;
    gif->delay = 0;
    uint8_t lct_flag = false;
    uint8_t interlace_flag = 0;
    uint8_t local_sort_flag = 0;
    uint8_t local_gct_bits = 0;
    uint8_t disposal_method = 0;
    while (true) {
        GIFStatus gif_err = read_next_bytes(gif, 1);
        if (gif_err) {
            clear_mem(gif);
            return gif_err;
        }
        if (gif->bytes.data[0] == 0x2C) {
            GIFStatus gif_err = read_next_bytes(gif, 9);
            if (gif_err) {
                clear_mem(gif);
                return gif_err;
            }
            start_x = gif->bytes.data[0] | (gif->bytes.data[1] << 8);
            start_y = gif->bytes.data[2] | (gif->bytes.data[3] << 8);
            a_width = gif->bytes.data[4] | (gif->bytes.data[5] << 8);
            a_height = gif->bytes.data[6] | (gif->bytes.data[7] << 8);
            lct_flag = (gif->bytes.data[8] >> 7) & 1;
            interlace_flag = (gif->bytes.data[8] >> 4) & 7;
            local_sort_flag = (gif->bytes.data[8] >> 3) & 1;
            local_gct_bits = gif->bytes.data[8] & 7;
            if (lct_flag) {
                colors_size = 1 << (local_gct_bits + 1);
                colors = (uint32_t*)malloc(sizeof(uint32_t) * colors_size);
                if (colors == NULL) {
                    clear_mem(gif);
                    return GIF_NO_MEMORY_FOR_LCT;
                }
                GIFStatus gif_err = read_next_bytes(gif, colors_size * 3);
                if (gif_err) {
                    free(colors);
                    colors = NULL;
                    clear_mem(gif);
                    return gif_err;
                }
                for (size_t i = 0, j = 0; i < colors_size; ++i, j += 3) {
                    colors[i] = RGBA8(gif->bytes.data[j], gif->bytes.data[j + 1], gif->bytes.data[j + 2], 0xFF);
                }
            }
        } else if (gif->bytes.data[0] == 0x21) {
            while (true) {
                GIFStatus gif_err = read_next_bytes(gif, 1);
                if (gif_err) {
                    if (lct_flag) {
                        free(colors);
                        colors = NULL;
                    }
                    clear_mem(gif);
                    return gif_err;
                }
                uint8_t byte = gif->bytes.data[0];
                if (byte != 0x00) {
                    GIFStatus gif_err = read_next_bytes(gif, 1);
                    if (gif_err) {
                        if (lct_flag) {
                            free(colors);
                            colors = NULL;
                        }
                        clear_mem(gif);
                        return gif_err;
                    }
                    uint8_t size = gif->bytes.data[0];
                    if (size > 0) {
                        GIFStatus gif_err = read_next_bytes(gif, size);
                        if (gif_err) {
                            if (lct_flag) {
                                free(colors);
                                colors = NULL;
                            }
                            clear_mem(gif);
                            return gif_err;
                        }
                        if (byte == 0xF9) {
                            transparent_id = gif->bytes.data[3];
                            gif->delay = (gif->bytes.data[1] | (gif->bytes.data[2] << 8)) * 10;
                            disposal_method = ((gif->bytes.data[1] >> 2) & 0x7);
                            if (disposal_method == 3 && gif->fframe != NULL && gif->frame != NULL) {
                                memcpy(gif->frame, gif->fframe, sizeof(uint32_t) * gif->width * gif->height);
                            }
                        } else if (byte == 0x01) {
                        } else if (byte == 0xFF) {
                            gif_err = read_next_bytes(gif, 1);
                            if (gif_err) {
                                if (lct_flag) {
                                    free(colors);
                                    colors = NULL;
                                }
                                clear_mem(gif);
                                return gif_err;
                            }
                            uint8_t size = gif->bytes.data[0];
                            if (size > 0) {
                                GIFStatus gif_err = read_next_bytes(gif, size);
                                if (gif_err) {
                                    if (lct_flag) {
                                        free(colors);
                                        colors = NULL;
                                    }
                                    clear_mem(gif);
                                    return gif_err;
                                }
                            }
                        } else if (byte == 0xFE) {
                        }
                    }
                } else {
                    break;
                }
            }
        } else if (gif->bytes.data[0] == 0x3B) {
            return GIF_END_FRAMES;
        } else {
            size_t code_ptr = 0;
            size_t fram_ptr = 0;
            uint16_t LZW_code_size = gif->bytes.data[0];
            uint16_t code_clr = 1 << LZW_code_size;
            uint16_t code_end = code_clr + 1;
            uint16_t code_prv = 0;
            uint16_t cmnd_cnt = code_end + 1;
            uint16_t size_pow = LZW_code_size + 1;
            uint16_t code = 0;
            int16_t byte_prv = -1;
            int16_t byte_pprv = -1;
            int8_t is_cleared = false;
            uint8_t eoi = false;
            struct command* commands = (struct command*)malloc(sizeof(struct command) * (1 << size_pow));
            for (uint16_t i = 0; i < 1 << size_pow; ++i) {
                commands[i].codes = (uint16_t*)malloc(sizeof(uint16_t));
                if (commands[i].codes == NULL) {
                    if (lct_flag) {
                        free(colors);
                        colors = NULL;
                    }
                    for (uint16_t j = 0; j < i; ++j) {
                        free(commands[j].codes);
                    }
                    free(commands);
                    clear_mem(gif);
                    return GIF_NO_MEMORY;
                }
                commands[i].size = 1;
                commands[i].codes[0] = i;
            }
            while (true) {
                GIFStatus gif_err = read_next_bytes(gif, 1);
                if (gif_err) {
                    if (lct_flag) {
                        free(colors);
                        colors = NULL;
                    }
                    for (uint16_t i = 0; i < 1 << size_pow; ++i) {
                        free(commands[i].codes);
                    }
                    free(commands);
                    clear_mem(gif);
                    return gif_err;
                }
                size_t size = gif->bytes.data[0];
                if (size > 0) {
                    GIFStatus gif_err = read_next_bytes(gif, size);
                    if (gif_err) {
                        if (lct_flag) {
                            free(colors);
                            colors = NULL;
                        }
                        for (uint16_t i = 0; i < 1 << size_pow; ++i) {
                            free(commands[i].codes);
                        }
                        free(commands);
                        clear_mem(gif);
                        return gif_err;
                    }
                    if (byte_pprv >= 0) {
                        if (byte_prv < 0) {
                            gif->bytes.data = (uint8_t*)realloc(gif->bytes.data, sizeof(uint8_t) * (gif->bytes.size + 1));
                            gif->bytes.size++;
                            size++;
                            for (size_t i = size - 1; i > 0; --i) {
                                gif->bytes.data[i] = gif->bytes.data[i - 1];
                            }
                            gif->bytes.data[0] = byte_pprv;
                        } else {
                            gif->bytes.data = (uint8_t*)realloc(gif->bytes.data, sizeof(uint8_t) * (gif->bytes.size + 2));
                            gif->bytes.size += 2;
                            size += 2;
                            for (size_t i = size - 1; i > 1; --i) {
                                gif->bytes.data[i] = gif->bytes.data[i - 2];
                            }
                            gif->bytes.data[0] = byte_pprv;
                            gif->bytes.data[1] = byte_prv;
                        }
                    } else {
                        code_ptr = 0;
                    }
                    do {
                        code = (gif->bytes.data[code_ptr / 8] >> (code_ptr % 8)) & (~(0xFF << size_pow));
                        if ((size_pow + code_ptr - 1) / 8 - code_ptr / 8 > 0) {
                            code |= (gif->bytes.data[code_ptr / 8 + 1] & (~(0xFF << (size_pow - (8 - (code_ptr % 8)))))) << (8 - (code_ptr % 8));
                            if ((size_pow + code_ptr - 1) / 8 - code_ptr / 8 > 1) {
                                code |= (gif->bytes.data[code_ptr / 8 + 2] & (~(0xFF << (size_pow - (16 - (code_ptr % 8)))))) << (16 - (code_ptr % 8));
                            }
                        }
                        code_ptr += size_pow;
                        if (code == code_clr) {
                            for (uint16_t i = 0; i < 1 << size_pow; ++i) {
                                free(commands[i].codes);
                            }
                            free(commands);
                            size_pow = LZW_code_size + 1;
                            commands = (struct command*)malloc(sizeof(struct command) * (1 << size_pow));
                            for (int i = 0; i < 1 << size_pow; ++i) {
                                commands[i].codes = (uint16_t*)malloc(sizeof(uint16_t));
                                if (commands[i].codes == NULL) {
                                    if (lct_flag) {
                                        free(colors);
                                        colors = NULL;
                                    }
                                    for (uint16_t j = 0; j < i; ++j) {
                                        free(commands[j].codes);
                                    }
                                    free(commands);
                                    clear_mem(gif);
                                    return GIF_NO_MEMORY;
                                }
                                commands[i].size = 1;
                                commands[i].codes[0] = i;
                            }
                            cmnd_cnt = code_end + 1;
                            is_cleared = true;
                        } else if (code == code_end) {
                            eoi = true;
                            break;
                        } else {
                            if (is_cleared) {
                                if (code != transparent_id || disposal_method == 2) {
                                    uint16_t line = (fram_ptr / a_width) + start_y;
                                    if (interlace_flag) {
                                        if (line * 8 < a_height) {
                                            line *= 8;
                                        } else {
                                            line -= a_height / 8;
                                            if (line * 8 + 4 < a_height) {
                                                line = line * 8 + 4;
                                            } else {
                                                line -= (a_height - 4) / 8 + 1;
                                                if (line * 4 + 2 < a_height) {
                                                    line = line * 4 + 2;
                                                } else {
                                                    line -= (a_height - 2) / 4 + 1;
                                                    line = line * 2 + 1;
                                                }
                                            }
                                        }
                                    }
                                    if (code == transparent_id) {
                                        if (transparent_id != gif->bkg_color) {
                                            gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = colors[gif->bkg_color];
                                        } else {
                                            gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = 0;
                                        }
                                    } else {
                                        gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = colors[code];
                                    }
                                }
                                fram_ptr++;
                                is_cleared = false;
                            } else {
                                int16_t k = -1;
                                if (code < cmnd_cnt) {
                                    for (size_t i = 0; i < commands[code].size; ++i) {
                                        uint16_t color_id = commands[code].codes[i];
                                        if (k < 0) {
                                            k = color_id;
                                        }
                                        if (color_id != transparent_id || disposal_method == 2) {
                                            uint16_t line = (fram_ptr / a_width) + start_y;
                                            if (interlace_flag) {
                                                if (line * 8 < a_height) {
                                                    line *= 8;
                                                } else {
                                                    line -= a_height / 8;
                                                    if (line * 8 + 4 < a_height) {
                                                        line = line * 8 + 4;
                                                    } else {
                                                        line -= (a_height - 4) / 8 + 1;
                                                        if (line * 4 + 2 < a_height) {
                                                            line = line * 4 + 2;
                                                        } else {
                                                            line -= (a_height - 2) / 4 + 1;
                                                            line = line * 2 + 1;
                                                        }
                                                    }
                                                }
                                            }
                                            if (color_id == transparent_id) {
                                                if (transparent_id != gif->bkg_color) {
                                                    gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = colors[gif->bkg_color];
                                                }
                                            } else {
                                                gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = colors[color_id];
                                            }
                                        }
                                        fram_ptr++;
                                    }
                                } else {
                                    for (size_t i = 0; i < commands[code_prv].size; ++i) {
                                        uint16_t color_id = commands[code_prv].codes[i];
                                        if (k < 0) {
                                            k = color_id;
                                        }
                                        if (color_id != transparent_id || disposal_method == 2) {
                                            uint16_t line = (fram_ptr / a_width) + start_y;
                                            if (interlace_flag) {
                                                if (line * 8 < a_height) {
                                                    line *= 8;
                                                } else {
                                                    line -= a_height / 8;
                                                    if (line * 8 + 4 < a_height) {
                                                        line = line * 8 + 4;
                                                    } else {
                                                        line -= (a_height - 4) / 8 + 1;
                                                        if (line * 4 + 2 < a_height) {
                                                            line = line * 4 + 2;
                                                        } else {
                                                            line -= (a_height - 2) / 4 + 1;
                                                            line = line * 2 + 1;
                                                        }
                                                    }
                                                }
                                            }
                                            if (color_id == transparent_id) {
                                                if (transparent_id != gif->bkg_color) {
                                                    gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = colors[gif->bkg_color];
                                                }
                                            } else {
                                                gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = colors[color_id];
                                            }
                                        }
                                        fram_ptr++;
                                    }
                                    if (k != transparent_id || disposal_method == 2) {
                                        uint16_t line = (fram_ptr / a_width) + start_y;
                                        if (interlace_flag) {
                                            if (line * 8 < a_height) {
                                                line *= 8;
                                            } else {
                                                line -= a_height / 8;
                                                if (line * 8 + 4 < a_height) {
                                                    line = line * 8 + 4;
                                                } else {
                                                    line -= (a_height - 4) / 8 + 1;
                                                    if (line * 4 + 2 < a_height) {
                                                        line = line * 4 + 2;
                                                    } else {
                                                        line -= (a_height - 2) / 4 + 1;
                                                        line = line * 2 + 1;
                                                    }
                                                }
                                            }
                                        }
                                        if (k == transparent_id) {
                                            if (transparent_id != gif->bkg_color) {
                                                gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = colors[gif->bkg_color];
                                            }
                                        } else {
                                            gif->frame[line * gif->width + (fram_ptr % a_width + start_x)] = colors[k];
                                        }
                                    }
                                    fram_ptr++;
                                }
                                if (code > cmnd_cnt) {
                                    return GIF_ERROR;
                                }
                                if (cmnd_cnt < 1 << size_pow) {
                                    commands[cmnd_cnt].codes = (uint16_t*)realloc(commands[cmnd_cnt].codes, sizeof(uint16_t) * (commands[code_prv].size + 1));
                                    if (commands[cmnd_cnt].codes == NULL) {
                                        if (lct_flag) {
                                            free(colors);
                                            colors = NULL;
                                        }
                                        for (uint16_t j = 0; j < cmnd_cnt; ++j) {
                                            free(commands[j].codes);
                                        }
                                        free(commands);
                                        clear_mem(gif);
                                        return GIF_NO_MEMORY;
                                    }
                                    memcpy(commands[cmnd_cnt].codes, commands[code_prv].codes, sizeof(uint16_t) * commands[code_prv].size);
                                    commands[cmnd_cnt].codes[commands[code_prv].size] = k;
                                    commands[cmnd_cnt].size = commands[code_prv].size + 1;
                                    cmnd_cnt++;
                                }
                            }
                        }
                        if (cmnd_cnt >= 1 << size_pow) {
                            size_pow++;
                            if (size_pow == 13) {
                                size_pow = 12;
                            } else {
                                commands = (struct command*)realloc(commands, sizeof(struct command) * (1 << size_pow));
                                for (int i = 1 << (size_pow - 1); i < 1 << size_pow; ++i) {
                                    commands[i].codes = (uint16_t*)malloc(sizeof(uint16_t));
                                    if (commands[i].codes == NULL) {
                                        if (lct_flag) {
                                            free(colors);
                                            colors = NULL;
                                        }
                                        for (uint16_t j = 0; j < i; ++j) {
                                            free(commands[j].codes);
                                        }
                                        free(commands);
                                        clear_mem(gif);
                                        return GIF_NO_MEMORY;
                                    }
                                    commands[i].size = 1;
                                    commands[i].codes[0] = i;
                                }
                            }
                        }
                        code_prv = code;
                        if (code_ptr + size_pow > size * 8) {
                            break;
                        }
                    } while (true);
                    byte_prv = -1;
                    byte_pprv = -1;
                    if (code_ptr / 8 < size) {
                        byte_pprv = gif->bytes.data[code_ptr / 8];
                        if (code_ptr / 8 + 1 < size) {
                            byte_prv = gif->bytes.data[code_ptr / 8 + 1];
                        }
                        code_ptr %= 8;
                    }
                    if (eoi) {
                        GIFStatus gif_err = read_next_bytes(gif, 1);
                        if (gif_err) {
                            if (lct_flag) {
                                free(colors);
                                colors = NULL;
                            }
                            for (uint16_t i = 0; i < 1 << size_pow; ++i) {
                                free(commands[i].codes);
                            }
                            free(commands);
                            clear_mem(gif);
                            return gif_err;
                        }
                        if (gif->bytes.data[0] != 0x00) {
                            return GIF_OK;
                        }
                        break;
                    }
                } else {
                    break;
                }
            }
            if (lct_flag) {
                free(colors);
                colors = NULL;
            }
            for (uint16_t i = 0; i < 1 << size_pow; ++i) {
                free(commands[i].codes);
            }
            free(commands);
            break;
        }
    }
    if (disposal_method < 2) {
        if (gif->fframe == NULL) {
            gif->fframe = (uint32_t*)malloc(sizeof(uint32_t) * gif->width * gif->height);
        }
        memcpy(gif->fframe, gif->frame, sizeof(uint32_t) * gif->width * gif->height);
    }
    return GIF_OK;
}

GIFStatus FreeGIF(struct GIFObject* gif) {
    if (gif == NULL) {
        return GIF_GIF_NULL_PTR_ERROR;
    }
    if (gif->chunk.data != NULL) {
        free(gif->chunk.data);
        gif->chunk.i = 0;
        gif->chunk.size = 0;
        gif->chunk.data = NULL;
        gif->chunk.is_last = false;
    }
    if (gif->gct != NULL) {
        free(gif->gct);
        gif->gct = NULL;
        gif->gct_size = 0;
    }
    if (gif->frame != NULL) {
        free(gif->frame);
        gif->frame = NULL;
    }
    if (gif->fframe != NULL) {
        free(gif->fframe);
        gif->fframe = NULL;
    }
    if (gif->descriptor != NULL) {
        if (gif->handler->term_f(gif->descriptor)) {
            free(gif->descriptor);
            gif->descriptor = NULL;
            return GIF_ERROR;
        } else {
            free(gif->descriptor);
            gif->descriptor = NULL;
        }
    }
    if (gif->bytes.data != NULL) {
        free(gif->bytes.data);
        gif->bytes.data = NULL;
    }
    return GIF_OK;
}

#undef false
#undef true