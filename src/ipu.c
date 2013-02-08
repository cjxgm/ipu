// vim: noet ts=4 sw=4 sts=0
#include "ipu.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************************
 *
 * stack
 *
 */

static IpuStack * _S;


IpuStack * ipu_stack_select(IpuStack * S)
{
	IpuStack * old_S = _S;
	_S = S;
	return old_S;
}

void ipu_stack_push(Image * I)
{
	pack_add_tail(_S, I);
}

Image * ipu_stack_pop()
{
	if (ipu_stack_is_empty()) return NULL;
	PackElement * pe = (PackElement *)_S->prev;
	Image * I = (Image *)pe->data;
	pack_delete(pe);
	return I;
}

inline Image * ipu_stack_top()
{
	if (ipu_stack_is_empty()) return NULL;
	PackElement * pe = (PackElement *)_S->prev;
	return (Image *)pe->data;
}

inline int ipu_stack_is_empty()
{
	return (pack_length(_S) == 0);
}


/***************************************************
 *
 * image manipulation
 *
 */

inline Image * ipu_image_new()
{
	CREATE(Image, I);
	return I;
}

inline void ipu_image_free(Image * I)
{
	free(I);
}


void ipu_color(float r, float g, float b)
{
	$_(I, ipu_image_new());
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) = r;
			ipu_at(I, x, y, 1) = g;
			ipu_at(I, x, y, 2) = b;
		}
	ipu_stack_push(I);
}

void ipu_pixel(float r, float g, float b, int npoint, int seed)
{
	$_(I, ipu_stack_top());
	srand(seed);
	while (npoint--) {
		int x = rand();
		int y = rand();
		ipu_at(I, x, y, 0) = r;
		ipu_at(I, x, y, 1) = g;
		ipu_at(I, x, y, 2) = b;
	}
}

void ipu_blur_x(int radius)
{
	$_(I, ipu_stack_pop());
	$_(new_I, ipu_image_new());
	int y, x, t;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			float color[4] = {0, 0, 0, radius*2+1};
			for (t=x-radius; t<=x+radius; t++) {
				color[0] += ipu_at(I, t, y, 0);
				color[1] += ipu_at(I, t, y, 1);
				color[2] += ipu_at(I, t, y, 2);
			}
			ipu_at(new_I, x, y, 0) = color[0] / color[3];
			ipu_at(new_I, x, y, 1) = color[1] / color[3];
			ipu_at(new_I, x, y, 2) = color[2] / color[3];
		}
	ipu_image_free(I);
	ipu_stack_push(new_I);
}

void ipu_blur_y(int radius)
{
	$_(I, ipu_stack_pop());
	$_(new_I, ipu_image_new());
	int y, x, t;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			float color[4] = {0, 0, 0, radius*2+1};
			for (t=y-radius; t<=y+radius; t++) {
				color[0] += ipu_at(I, x, t, 0);
				color[1] += ipu_at(I, x, t, 1);
				color[2] += ipu_at(I, x, t, 2);
			}
			ipu_at(new_I, x, y, 0) = color[0] / color[3];
			ipu_at(new_I, x, y, 1) = color[1] / color[3];
			ipu_at(new_I, x, y, 2) = color[2] / color[3];
		}
	ipu_image_free(I);
	ipu_stack_push(new_I);
}

void ipu_blur(int radius, float angle_in_degree)
{
	ipu_blur_x(radius * cos(angle_in_degree * M_PI / 180));
	ipu_blur_y(radius * sin(angle_in_degree * M_PI / 180));
}

void ipu_mul(float r, float g, float b)
{
	$_(I, ipu_stack_top());
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) *= r;
			ipu_at(I, x, y, 1) *= g;
			ipu_at(I, x, y, 2) *= b;
		}
}

void ipu_dup()
{
	$_(I, ipu_stack_top());
	$_(new_I, ipu_image_new());
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(new_I, x, y, 0) = ipu_at(I, x, y, 0);
			ipu_at(new_I, x, y, 1) = ipu_at(I, x, y, 1);
			ipu_at(new_I, x, y, 2) = ipu_at(I, x, y, 2);
		}
	ipu_stack_push(new_I);
}


void ipu_mix_add()
{
	$_(I2, ipu_stack_pop());
	$_(I , ipu_stack_top());
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) += ipu_at(I2, x, y, 0);
			ipu_at(I, x, y, 1) += ipu_at(I2, x, y, 1);
			ipu_at(I, x, y, 2) += ipu_at(I2, x, y, 2);
		}
}

void ipu_mix_sub();
void ipu_mix_mul();
void ipu_mix_div();

/***************************************************
 *
 * file io
 *
 */

void ipu_save_to_ppm(const char * filename)
{
#define CLAMP(v, f, t) ({ \
	typeof(v) __$v = (v); \
	typeof(f) __$f = (f); \
	typeof(t) __$t = (t); \
	(__$v < __$f ? __$f : (__$v > __$t ? __$t : __$v)); \
})
	FILE * fp = fopen(filename, "w");
	$_(I, ipu_stack_top());
	fprintf(fp, "P6\n256 256\n255\n");
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			unsigned char color[3] = {
				CLAMP(ipu_at(I, x, y, 0), 0, 1) * 255,
				CLAMP(ipu_at(I, x, y, 1), 0, 1) * 255,
				CLAMP(ipu_at(I, x, y, 2), 0, 1) * 255,
			};
			fwrite(color, sizeof(color), 1, fp);
		}
	fclose(fp);
#undef CLAMP
}

