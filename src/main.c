
#include <stdio.h>
#include "ipu.h"
#include "ui.h"

int main()
{
	ipu_stack_select(ipu_stack_new());

	ipu_color(0, 0, 0);
	ipu_pixel(0.5, 0.8, 0.4, 1000, 1995);
	ipu_mul(10, 10, 10);
	ipu_blur_x(20);
	ipu_blur_x(20);
	ipu_blur_x(20);

	ipu_dup();
	ipu_blur_y(5);
	ipu_blur_y(5);
	ipu_blur_y(5);
	ipu_mul(4, 4, 4);
	ipu_mix_add();

	ipu_color(0, 0, 0);
	ipu_pixel(0.8, 0.8, 0.8, 200, 1996);
	ipu_blur_x(5);
	ipu_blur_x(5);
	ipu_blur_x(5);
	ipu_blur_y(5);
	ipu_blur_y(5);
	ipu_blur_y(5);
	ipu_mul(60, 60, 60);
	ipu_clamp();
	ipu_level(0.4, 1);
	ipu_clamp();
	ipu_mix_add();

	ipu_scale(0.5, 0.5);
	ipu_move(30, 0);
	ipu_rotate(45);

	ui_run();

	return 0;
}

