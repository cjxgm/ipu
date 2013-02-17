// vim: noet ts=4 sw=4 sts=0
#include "ipu.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pool.h"

/***************************************************
 *
 * misc
 *
 */

static Pool * _P_image;

bool ipu_init()
{
	_P_image = pool_new(0x100);	// if this fail, let it crash the app!
	ipu_stack_select(ipu_stack_new());	// if this fail, let it crash the app!
	return false;
}


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

void ipu_stack_push(IpuImage * I)
{
	pack_add_tail(_S, I);
}

IpuImage * ipu_stack_pop()
{
	if (ipu_stack_is_empty()) return NULL;
	PackElement * pe = (PackElement *)_S->prev;
	IpuImage * I = (IpuImage *)pe->data;
	pack_delete(pe);
	return I;
}

IpuImage * ipu_stack_top()
{
	if (ipu_stack_is_empty()) return NULL;
	PackElement * pe = (PackElement *)_S->prev;
	return (IpuImage *)pe->data;
}

int ipu_stack_is_empty()
{
	return (pack_length(_S) == 0);
}

void ipu_stack_clear()
{
	IpuImage * I;
	while ((I = ipu_stack_pop()))
		ipu_image_free(I);
}

int ipu_stack_length()
{
	return pack_length(_S);
}


/***************************************************
 *
 * image manipulation
 *
 */

IpuImage * ipu_image_new()
{
	$_(I, pool_get(_P_image));
	if (!I) I = new(IpuImage);
	return I;
}

void ipu_image_free(IpuImage * I)
{
	if (pool_put(_P_image, I))	// full
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

// perlin noise
// see: http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
// s[xy] = scale of [xy]
bool ipu_pnoise(float r, float g, float b, float ox, float oy,
		float sx, float sy, float persistence, int nrecursion)
{
	// return value ranged [-1, 1]
	float noise(int x, int y)
	{
		int n = y*57 + x;
		n = (n<<13) ^ n;
		return (1.0 - (((n*n*15731 + 789221)*n +
						1376312589) & 0x7fffffff) / 1073741824.0); 
	}

	// super-sampling noise
	float ssnoise(float x, float y)
	{
		float n = noise(x, y) / 4.0;
		n += (noise(x-1, y) +
				noise(x+1, y) +
				noise(x, y-1) +
				noise(x, y+1)) / 8.0;
		n += (noise(x-1, y-1) +
				noise(x+1, y-1) +
				noise(x-1, y+1) +
				noise(x+1, y+1)) / 16.0;
		return n;
	}

	// cosine interpolation
	inline float interpolate(float a, float b, float t)
	{
		float f = (1 - cosf(t * M_PI)) * 0.5;
		return a*(1-f) + b*f;
	}

	// interpolated noise
	inline float inoise(float x, float y)
	{
		int   ix = x;			// integer    part of x
		float fx = x - ix;		// fractional part of x
		int   iy = y;			// integer    part of y
		float fy = y - iy;		// fractional part of y
		float n1 = interpolate(ssnoise(ix, iy  ), ssnoise(ix+1, iy  ), fx);
		float n2 = interpolate(ssnoise(ix, iy+1), ssnoise(ix+1, iy+1), fx);
		return interpolate(n1, n2, fy);
	}

	// perlin noise
	inline float pnoise(float x, float y)
	{
		float n = 0;
		for (int i=0; i<nrecursion; i++) {
			float f = powf(2, i);
			float a = powf(persistence, i);
			n += a * inoise(x*f, y*f);
		}
		return n;
	}

	$_(I, ipu_image_new());	// if this fail, let it crash the app!
	for (int y=0; y<256; y++)
		for (int x=0; x<256; x++) {
			float a = (pnoise(ox + x/sx, oy + y/sy) + 1) / 2;
			ipu_at(I, x, y, 0) = r*a;
			ipu_at(I, x, y, 1) = g*a;
			ipu_at(I, x, y, 2) = b*a;
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

bool ipu_circle(float r, float g, float b, float ox, float oy,
				float radius, bool super_sampling)
{
	$_(I, ipu_stack_top());
	if (!I) return true;

	radius = radius * radius;
	if (super_sampling)
		for (int y=0; y<256; y++)
			for (int x=0; x<256; x++) {
				float alpha = 0;

				void test(float offx, float offy, float ratio)
				{
					float dx = x - ox + offx;
					float dy = y - oy + offy;
					if (dx*dx + dy*dy > radius) return;
					alpha += ratio;
				}

				test(0, 0, 1/4.0);
				test(-0.5, 0, 1/8.0);
				test(+0.5, 0, 1/8.0);
				test(0, -0.5, 1/8.0);
				test(0, +0.5, 1/8.0);
				test(-0.5, -0.5, 1/16.0);
				test(+0.5, -0.5, 1/16.0);
				test(-0.5, +0.5, 1/16.0);
				test(+0.5, +0.5, 1/16.0);

				if (alpha) {
					ipu_at(I, x, y, 0) *= 1-alpha;
					ipu_at(I, x, y, 1) *= 1-alpha;
					ipu_at(I, x, y, 2) *= 1-alpha;
					ipu_at(I, x, y, 0) += r*alpha;
					ipu_at(I, x, y, 1) += g*alpha;
					ipu_at(I, x, y, 2) += b*alpha;
				}
			}
	else
		for (int y=0; y<256; y++)
			for (int x=0; x<256; x++) {
				float dx = x - ox;
				float dy = y - oy;
				if (dx*dx + dy*dy > radius) continue;
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
	int y, x;
	for (y=0; y<256; y++) {
		float color[4] = {0, 0, 0, radius*2+1};
		for (x=-radius; x<=radius; x++) {
			color[0] += ipu_at(I, x, y, 0);
			color[1] += ipu_at(I, x, y, 1);
			color[2] += ipu_at(I, x, y, 2);
		}
		ipu_at(new_I, 0, y, 0) = color[0] / color[3];
		ipu_at(new_I, 0, y, 1) = color[1] / color[3];
		ipu_at(new_I, 0, y, 2) = color[2] / color[3];
		for (x=1; x<256; x++) {
			ipu_at(new_I, x, y, 0) = ipu_at(new_I, x-1, y, 0) +
				(ipu_at(I, x+radius, y, 0) -
				 ipu_at(I, x-radius-1, y, 0)) / color[3];
			ipu_at(new_I, x, y, 1) = ipu_at(new_I, x-1, y, 1) +
				(ipu_at(I, x+radius, y, 1) -
				 ipu_at(I, x-radius-1, y, 1)) / color[3];
			ipu_at(new_I, x, y, 2) = ipu_at(new_I, x-1, y, 2) +
				(ipu_at(I, x+radius, y, 2) -
				 ipu_at(I, x-radius-1, y, 2)) / color[3];
		}
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

	int y, x;
	for (x=0; x<256; x++) {
		float color[4] = {0, 0, 0, radius*2+1};
		for (y=-radius; y<=radius; y++) {
			color[0] += ipu_at(I, x, y, 0);
			color[1] += ipu_at(I, x, y, 1);
			color[2] += ipu_at(I, x, y, 2);
		}
		ipu_at(new_I, x, 0, 0) = color[0] / color[3];
		ipu_at(new_I, x, 0, 1) = color[1] / color[3];
		ipu_at(new_I, x, 0, 2) = color[2] / color[3];
		for (y=1; y<256; y++) {
			ipu_at(new_I, x, y, 0) = ipu_at(new_I, x, y-1, 0) +
				(ipu_at(I, x, y+radius, 0) -
				 ipu_at(I, x, y-radius-1, 0)) / color[3];
			ipu_at(new_I, x, y, 1) = ipu_at(new_I, x, y-1, 1) +
				(ipu_at(I, x, y+radius, 1) -
				 ipu_at(I, x, y-radius-1, 1)) / color[3];
			ipu_at(new_I, x, y, 2) = ipu_at(new_I, x, y-1, 2) +
				(ipu_at(I, x, y+radius, 2) -
				 ipu_at(I, x, y-radius-1, 2)) / color[3];
		}
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
	memcpy(new_I, I, sizeof(IpuImage));
	ipu_stack_push(new_I);

	return false;
}

bool ipu_ignore()
{
	$_(I, ipu_stack_pop());
	if (!I) return true;

	ipu_image_free(I);

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

bool ipu_desaturate()
{
#define R_RATIO		0.2126f
#define G_RATIO		0.7152f
#define B_RATIO		0.0722f
	$_(I, ipu_stack_top());
	if (!I) return true;

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++)
			ipu_at(I, x, y, 0) =
			ipu_at(I, x, y, 1) =
			ipu_at(I, x, y, 2) =
					ipu_at(I, x, y, 0)*R_RATIO +
					ipu_at(I, x, y, 1)*G_RATIO +
					ipu_at(I, x, y, 2)*B_RATIO;

	return false;
#undef R_RATIO
#undef G_RATIO
#undef B_RATIO
}


//////////////////////////////// map
bool ipu_bump()
{
	if (ipu_desaturate()) return true;

	$_(I2, ipu_stack_pop());
	if (!I2) return true;		// XXX: I think this will NEVER happen.

	$_(I, ipu_stack_top());
	if (!I) {
		ipu_image_free(I2);
		return true;
	}

	for (int y=0; y<256; y++)
		for (int x=0; x<256; x++) {
			float brightness_l = ipu_at(I2, x-1, y, 0);
			float brightness_r = ipu_at(I2, x+1, y, 0);
			float brightness_u = ipu_at(I2, x, y-1, 0);
			float brightness_d = ipu_at(I2, x, y+1, 0);
			float bx = (brightness_l - brightness_r + 1) / 2;
			float by = (brightness_u - brightness_d + 1) / 2;
			float t = sqrtf(bx*bx + by*by) * sqrtf(2);
			ipu_at(I, x, y, 0) *= t;
			ipu_at(I, x, y, 1) *= t;
			ipu_at(I, x, y, 2) *= t;
		}

	ipu_image_free(I2);

	return false;
}

bool ipu_displace(float sx, float sy)
{
	if (ipu_desaturate()) return true;

	$_(I2, ipu_stack_pop());
	if (!I2) return true;		// XXX: I think this will NEVER happen.

	$_(I, ipu_stack_pop());
	if (!I) {
		ipu_image_free(I2);
		return true;
	}

	$_(new_I, ipu_image_new());	// if this fail, let it crash the app!

	for (int y=0; y<256; y++)
		for (int x=0; x<256; x++) {
			float disp = ipu_at(I2, x, y, 0)*2 - 1;
			int dx = x + disp * sx;
			int dy = y + disp * sy;
			ipu_at(new_I, x, y, 0) = ipu_at(I, dx, dy, 0);
			ipu_at(new_I, x, y, 1) = ipu_at(I, dx, dy, 1);
			ipu_at(new_I, x, y, 2) = ipu_at(I, dx, dy, 2);
		}

	ipu_image_free(I2);
	ipu_image_free(I);
	ipu_stack_push(new_I);

	return false;
}


//////////////////////////////// transform
bool ipu_transform(float ox, float oy, float xx, float xy,
					float yx, float yy, bool super_sampling)
{
	$_(I, ipu_stack_pop());
	if (!I) return true;

	$_(new_I, ipu_image_new());	// if this fail, let it crash the app!

	if (super_sampling)
		for (int y=0; y<256; y++)
			for (int x=0; x<256; x++) {
				float color[] = {0, 0, 0};

				inline void sample(float tx, float ty, float ratio)
				{
					int nx = ox + xx*tx + yx*ty;
					int ny = oy + xy*tx + yy*ty;
					color[0] += ipu_at(I, nx, ny, 0) * ratio;
					color[1] += ipu_at(I, nx, ny, 1) * ratio;
					color[2] += ipu_at(I, nx, ny, 2) * ratio;
				}

				sample(x, y, 1.0/4.0);
				sample(x-0.5, y, 1.0/8.0);
				sample(x+0.5, y, 1.0/8.0);
				sample(x, y-0.5, 1.0/8.0);
				sample(x, y+0.5, 1.0/8.0);
				sample(x-0.5, y-0.5, 1.0/16.0);
				sample(x+0.5, y-0.5, 1.0/16.0);
				sample(x-0.5, y+0.5, 1.0/16.0);
				sample(x+0.5, y+0.5, 1.0/16.0);

				ipu_at(new_I, x, y, 0) = color[0];
				ipu_at(new_I, x, y, 1) = color[1];
				ipu_at(new_I, x, y, 2) = color[2];
			}
	else
		for (int y=0; y<256; y++)
			for (int x=0; x<256; x++) {
				int nx = ox + xx*x + yx*y;
				int ny = oy + xy*x + yy*y;
				ipu_at(new_I, x, y, 0) = ipu_at(I, nx, ny, 0);
				ipu_at(new_I, x, y, 1) = ipu_at(I, nx, ny, 1);
				ipu_at(new_I, x, y, 2) = ipu_at(I, nx, ny, 2);
			}

	ipu_image_free(I);
	ipu_stack_push(new_I);

	return false;
}

bool ipu_move(float x, float y, bool super_sampling)
{
	return ipu_transform(-x, -y, 1, 0, 0, 1, super_sampling);
}

bool ipu_scale(float x, float y, bool super_sampling)
{
	return ipu_transform(0, 0, 1/x, 0, 0, 1/y, super_sampling);
}

bool ipu_rotate(float angle_in_degree, bool super_sampling)
{
	float c = cos(angle_in_degree * M_PI / 180);
	float s = sin(angle_in_degree * M_PI / 180);
	return ipu_transform(0, 0, c, -s, s, c, super_sampling);
}


//////////////////////////////// mix
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

	ipu_image_free(I2);

	return false;
}

bool ipu_mix_sub()
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
			ipu_at(I, x, y, 0) -= ipu_at(I2, x, y, 0);
			ipu_at(I, x, y, 1) -= ipu_at(I2, x, y, 1);
			ipu_at(I, x, y, 2) -= ipu_at(I2, x, y, 2);
		}

	ipu_image_free(I2);

	return false;
}

bool ipu_mix_mul()
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
			ipu_at(I, x, y, 0) *= ipu_at(I2, x, y, 0);
			ipu_at(I, x, y, 1) *= ipu_at(I2, x, y, 1);
			ipu_at(I, x, y, 2) *= ipu_at(I2, x, y, 2);
		}

	ipu_image_free(I2);

	return false;
}

bool ipu_mix_div()
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
			ipu_at(I, x, y, 0) /= ipu_at(I2, x, y, 0);
			ipu_at(I, x, y, 1) /= ipu_at(I2, x, y, 1);
			ipu_at(I, x, y, 2) /= ipu_at(I2, x, y, 2);
		}

	ipu_image_free(I2);

	return false;
}


/***************************************************
 *
 * io
 *
 */

unsigned char * ipu_ppm_get(size_t * size)
{
#define PPM_SIZE	(3+8+4 + 256*256*3)
	if (ipu_dup() || ipu_clamp()) return NULL;

	$_(I, ipu_stack_pop());
	if (!I) return NULL;

	// if this fail, let it crash the app!
	create(unsigned char, ppm, * PPM_SIZE);
	memcpy(ppm, "P6\n256 256\n255\n", 3+8+4);

	int y, x;
	for (y=0; y<256; y++)
		for (x=0; x<256; x++) {
			ppm[3+8+4 + ((y<<8)|x)*3 + 0] = ipu_at(I, x, y, 0) * 255;
			ppm[3+8+4 + ((y<<8)|x)*3 + 1] = ipu_at(I, x, y, 1) * 255;
			ppm[3+8+4 + ((y<<8)|x)*3 + 2] = ipu_at(I, x, y, 2) * 255;
		}

	ipu_image_free(I);
	if (size) *size = PPM_SIZE;
	return ppm;
#undef PPM_SIZE
}

void ipu_ppm_free(unsigned char * ppm)
{
	free(ppm);
}

bool ipu_ppm_save_to_file(const char * filename)
{
	FILE * fp = fopen(filename, "w");
	if (!fp) return true;

// TODO: measure the performance
	size_t ppm_size;
	$_(ppm, ipu_ppm_get(&ppm_size));
	if (!ppm) {
		fclose(fp);
		return true;
	}

	fwrite(ppm, ppm_size, 1, fp);
	ipu_ppm_free(ppm);

	fclose(fp);
	return false;
/*
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
*/
}

