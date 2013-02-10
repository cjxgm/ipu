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

Image * ipu_stack_top()
{
	if (ipu_stack_is_empty()) return NULL;
	PackElement * pe = (PackElement *)_S->prev;
	return (Image *)pe->data;
}

int ipu_stack_is_empty()
{
	return (pack_length(_S) == 0);
}


/***************************************************
 *
 * image manipulation
 *
 */

Image * ipu_image_new()
{
	create(Image, I);
	return I;
}

void ipu_image_free(Image * I)
{
	free(I);
}


bool ipu_color(float r, float g, float b)
{
	$_(I, ipu_image_new());	// if this fail, let it crash the app!
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) = r;
			ipu_at(I, x, y, 1) = g;
			ipu_at(I, x, y, 2) = b;
		}
	ipu_stack_push(I);

	return false;
}

bool ipu_pixel(float r, float g, float b, int npoint, int seed)
{
	$_(I, ipu_stack_top());
	if (!I) return true;

	srand(seed);
	while (npoint--) {
		int x = rand();
		int y = rand();
		ipu_at(I, x, y, 0) = r;
		ipu_at(I, x, y, 1) = g;
		ipu_at(I, x, y, 2) = b;
	}

	return false;
}

bool ipu_blur_x(int radius)
{
	$_(I, ipu_stack_pop());
	if (!I) return true;

	$_(new_I, ipu_image_new());	// if this fail, let it crash the app!
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

	return false;
}

bool ipu_blur_y(int radius)
{
	$_(I, ipu_stack_pop());
	if (!I) return true;

	$_(new_I, ipu_image_new());	// if this fail, let it crash the app!
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

	return false;
}

bool ipu_blur(int radius, float angle_in_degree)
{
	return ipu_blur_x(radius * cos(angle_in_degree * M_PI / 180)) ||
			ipu_blur_y(radius * sin(angle_in_degree * M_PI / 180));
}

bool ipu_mul(float r, float g, float b)
{
	$_(I, ipu_stack_top());
	if (!I) return true;

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) *= r;
			ipu_at(I, x, y, 1) *= g;
			ipu_at(I, x, y, 2) *= b;
		}

	return false;
}

bool ipu_dup()
{
	$_(I, ipu_stack_top());
	if (!I) return true;

	$_(new_I, ipu_image_new());	// if this fail, let it crash the app!
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(new_I, x, y, 0) = ipu_at(I, x, y, 0);
			ipu_at(new_I, x, y, 1) = ipu_at(I, x, y, 1);
			ipu_at(new_I, x, y, 2) = ipu_at(I, x, y, 2);
		}
	ipu_stack_push(new_I);

	return false;
}

bool ipu_clamp()
{
#define CLAMP(v, f, t) ({ \
	typeof(v) __$v = (v); \
	typeof(f) __$f = (f); \
	typeof(t) __$t = (t); \
	(__$v < __$f ? __$f : (__$v > __$t ? __$t : __$v)); \
})
	$_(I, ipu_stack_top());
	if (!I) return true;

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) = CLAMP(ipu_at(I, x, y, 0), 0, 1);
			ipu_at(I, x, y, 1) = CLAMP(ipu_at(I, x, y, 1), 0, 1);
			ipu_at(I, x, y, 2) = CLAMP(ipu_at(I, x, y, 2), 0, 1);
		}

	return false;
#undef CLAMP
}

bool ipu_level(float f, float t)
{
#define LIRP(v, f, t, df, dt) ({ \
	typeof(v) __$v = (v); \
	typeof(f) __$f = (f); \
	typeof(t) __$t = (t); \
	typeof(df) __$df = (df); \
	typeof(dt) __$dt = (dt); \
	(__$v-__$f) / (__$t-__$f) * (__$dt-__$df) + __$df; \
})
	$_(I, ipu_stack_top());
	if (!I) return true;

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) = LIRP(ipu_at(I, x, y, 0), f, t, 0, 1);
			ipu_at(I, x, y, 1) = LIRP(ipu_at(I, x, y, 1), f, t, 0, 1);
			ipu_at(I, x, y, 2) = LIRP(ipu_at(I, x, y, 2), f, t, 0, 1);
		}

	return false;
#undef LIRP
}


bool ipu_mix_add()
{
	$_(I2, ipu_stack_pop());
	if (!I2) return true;

	$_(I, ipu_stack_top());
	if (!I) {
		ipu_image_free(I2);
		return true;
	}

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ipu_at(I, x, y, 0) += ipu_at(I2, x, y, 0);
			ipu_at(I, x, y, 1) += ipu_at(I2, x, y, 1);
			ipu_at(I, x, y, 2) += ipu_at(I2, x, y, 2);
		}

	return false;
}

bool ipu_mix_sub()
{
	return true;
}

bool ipu_mix_mul()
{
	return true;
}

bool ipu_mix_div()
{
	return true;
}


/***************************************************
 *
 * file io
 *
 */

bool ipu_save_to_ppm(const char * filename)
{
	FILE * fp = fopen(filename, "w");
	if (!fp) return true;

	if (ipu_dup() || ipu_clamp()) goto __fail;

	$_(I, ipu_stack_pop());
	if (!I) goto __fail;

	fprintf(fp, "P6\n256 256\n255\n");
	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			unsigned char color[3] = {
				ipu_at(I, x, y, 0) * 255,
				ipu_at(I, x, y, 1) * 255,
				ipu_at(I, x, y, 2) * 255,
			};
			fwrite(color, sizeof(color), 1, fp);
		}
	fclose(fp);

	ipu_image_free(I);
	return false;

__fail:
	fclose(fp);
	return true;
}

