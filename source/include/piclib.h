//code is written by creckeryop
#ifndef PICLIB_H
#define PICLIB_H

#include <vita2d.h>

typedef enum pic_format
{
	PIC_FORMAT_PNG = 0,
	PIC_FORMAT_BMP = 1,
	PIC_FORMAT_JPEG = 2,
	PIC_FORMAT_GIF = 3,
	PIC_FORMAT_UNKNOWN = 4
} pic_format;

struct gif_texture{
	vita2d_texture** texture;
	size_t *delay;
	size_t frames;
	size_t width;
	size_t height;
};

vita2d_texture *load_PIC_file(const char *filename, int x, int y, int width, int height);
vita2d_texture *load_PIC_file_downscaled(const char *filename, int level);
void get_PIC_resolution(const char *filename, int *dest_width, int *dest_height);
pic_format get_PIC_format(const char *filename);
struct gif_texture* load_GIF_file(const char *filename);

#endif