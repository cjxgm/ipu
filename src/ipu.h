// vim: noet ts=4 sw=4 sts=0
#ifndef __IPU__
#define __IPU__

#include "pack.h"

/***************************************************
 *
 * stack
 *
 */

typedef Pack IpuStack;

typedef struct Image
{
	float data[256*256*3];
}
Image;

#define ipu_stack_new()					((IpuStack *)pack_new())
IpuStack * ipu_stack_select(IpuStack * S);

void ipu_stack_push(Image * I);
Image * ipu_stack_pop();
Image * ipu_stack_top();
int ipu_stack_is_empty();

/***************************************************
 *
 * image manipulation
 *
 */

#define ipu_at(I, x, y, idx) \
	((I)->data[((((y) & 0xFF)<<8) | ((x) & 0xFF)) * 3 + (idx)])

Image * ipu_image_new();
void ipu_image_free(Image * I);

void ipu_color(float r, float g, float b);
void ipu_pixel(float r, float g, float b, int npoint, int seed);
void ipu_blur_x(int radius);
void ipu_blur_y(int radius);
void ipu_blur(int radius, float angle_in_degree);
void ipu_mul(float r, float g, float b);
void ipu_dup();
void ipu_clamp();
void ipu_level(float f, float t);

void ipu_mix_add();
void ipu_mix_sub();
void ipu_mix_mul();
void ipu_mix_div();

/***************************************************
 *
 * file io
 *
 */

void ipu_save_to_ppm(const char * filename);

#endif

