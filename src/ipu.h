// vim: noet ts=4 sw=4 sts=0
// Image Processing Unit
#ifndef __IPU__
#define __IPU__

#include "pack.h"

/***************************************************
 *
 * stack
 *
 */

typedef Pack IpuStack;

typedef struct IpuImage
{
	float data[256*256*3];
}
IpuImage;

#define ipu_stack_new()					((IpuStack *)pack_new())
IpuStack * ipu_stack_select(IpuStack * S);

void ipu_stack_push(IpuImage * I);
IpuImage * ipu_stack_pop();
IpuImage * ipu_stack_top();
int ipu_stack_is_empty();
void ipu_stack_clear();
int ipu_stack_length();

/***************************************************
 *
 * image manipulation
 *
 */

#define ipu_at(I, x, y, idx) \
	((I)->data[((((y) & 0xFF)<<8) | ((x) & 0xFF)) * 3 + (idx)])

IpuImage * ipu_image_new();
void ipu_image_free(IpuImage * I);

bool ipu_color(float r, float g, float b);
bool ipu_pixel(float r, float g, float b, int npoint, int seed);
bool ipu_circle(float r, float g, float b, float ox, float oy,
				float radius, bool super_sampling);
bool ipu_blur_x(int radius);
bool ipu_blur_y(int radius);
bool ipu_blur(int radius, float angle_in_degree);
bool ipu_mul(float r, float g, float b);
bool ipu_dup();
bool ipu_ignore();
bool ipu_clamp();
bool ipu_level(float f, float t);
bool ipu_bump();

// o[xy] is the new origin
// x[xy] is the new x-axis, y[xy] if the new y-axis
// with this function, you can do rotation, scaling(nearest), squeeze etc.
bool ipu_transform(float ox, float oy, float xx, float xy, float yx, float yy);
bool ipu_move(float x, float y);
bool ipu_scale(float x, float y);
bool ipu_rotate(float angle_in_degree);

bool ipu_mix_add();
bool ipu_mix_sub();
bool ipu_mix_mul();
bool ipu_mix_div();

/***************************************************
 *
 * io
 *
 */

// you should free the returned data with ipu_ppm_free by your self!
// <size> can be NULL if you don't need it
unsigned char * ipu_ppm_get(size_t * size);

void ipu_ppm_free(unsigned char * ppm);
bool ipu_ppm_save_to_file(const char * filename);

#endif

