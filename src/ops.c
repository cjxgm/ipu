
#include <stdio.h>
#include "ops.h"
#include "ui.h"
#include "ipu.h"

#define expand(x...)	x
#define operator($name, $prop_infos, $pull...) ({ \
	static PropInfo $prop_infos$[] = expand $prop_infos; \
	ui_register_operator($name, array_length($prop_infos$), \
			$prop_infos$, $(bool, (float v[]) $pull)); \
})

#define MIN		(-1e6)
#define MAX		(+1e6)

void ops_register_operators()
{
	operator("color", ({ 
		{ "r", 0, MIN, MAX, 1e-6, 1e-2 },
		{ "g", 0, MIN, MAX, 1e-6, 1e-2 },
		{ "b", 0, MIN, MAX, 1e-6, 1e-2 },
	}), {
		return ipu_color(v[0], v[1], v[2]);
	});

	operator("pixel", ({
		{ "r",		1,	MIN,	MAX,		1e-6,	1e-2	},
		{ "g",		1,	MIN,	MAX,		1e-6,	1e-2	},
		{ "b",		1,	MIN,	MAX,		1e-6,	1e-2	},
		{ "amount",	80,	0,		0xFFFF,		1,		1		},
		{ "seed",	0,	0,		0xFFFFFFFF,	1,		1		},
	}), {
		return ipu_pixel(v[0], v[1], v[2], v[3], v[4]);
	});

	operator("blur", ({
		{ "radius x",	50,	0,	0xFFFF,	1,		1		},
		{ "radius y",	0,	0,	0xFFFF,	1,		1		},
		{ "times",		4,	0,	0xFFFF,	1,		1		},
		{ "amplify",	60,	-1,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = false;
		int rx = v[0];
		int ry = v[1];
		int n  = v[2];
		if (rx) for (int i=0; i<n; i++) err |= ipu_blur_x(rx);
		if (ry) for (int i=0; i<n; i++) err |= ipu_blur_y(ry);
		err |= ipu_mul(v[3], v[3], v[3]);
		return err;
	});
}

