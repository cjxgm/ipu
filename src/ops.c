
#include <stdio.h>
#include "ops.h"
#include "ui.h"
#include "ipu.h"

#define expand(x...)	x
#define operator($name, $nprop, $prop_names, $poll...) ({ \
	static const char * $prop_names$[] = expand $prop_names; \
	ui_register_operator($name, $nprop, $prop_names$, \
			$(bool, (float props[]) $poll)); \
})

void ops_register_operators()
{
	operator("color", 3, ({ "r", "g", "b" }), {
		return ipu_color(props[0], props[1], props[2]);
	});

	operator("pixel", 5, ({ "r", "g", "b", "amount", "seed" }), {
		return ipu_pixel(props[0], props[1], props[2], props[3], props[4]);
	});

	operator("blur", 3, ({ "radius x", "radius y", "times" }), {
		bool err = false;
		int rx = props[0];
		int ry = props[1];
		int n  = props[2];
		if (rx > 0) for (int i=0; i<n; i++) err |= ipu_blur_x(rx);
		if (ry > 0) for (int i=0; i<n; i++) err |= ipu_blur_y(ry);
		return err;
	});
}

