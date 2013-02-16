
#include <stdio.h>
#include <math.h>
#include "ops.h"
#include "ui.h"
#include "ipu.h"

#define expand(x...)	x
#define operator($name, $prop_infos, $pull...) ({ \
	static const PropInfo $prop_infos$[] = expand $prop_infos; \
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

	operator("circle", ({
		{ "r",		1,		MIN,	MAX,		1e-6,	1e-2	},
		{ "g",		1,		MIN,	MAX,		1e-6,	1e-2	},
		{ "b",		1,		MIN,	MAX,		1e-6,	1e-2	},
		{ "x",		127,	MIN,	MAX,		1e-2,	1		},
		{ "y",		127,	MIN,	MAX,		1e-2,	1		},
		{ "radius",	80,		0,		0xFFFF,		1e-2,	1		},
		{ "super sampling",	0,	0,	1,			1,		1		},
	}), {
		return ipu_circle(v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
	});

	operator("blur", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
		{ "radius x",	50,	0,	0xFFFF,	1,		1		},
		{ "radius y",	0,	0,	0xFFFF,	1,		1		},
		{ "times",		4,	0,	0xFFFF,	1,		1		},
		{ "amplify",	60,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = false;
		int rx = v[1];
		int ry = v[2];
		int n  = v[3];
		if (v[0] != 1) err |= ipu_mul(v[0], v[0], v[0]);
		if (rx) for (int i=0; i<n; i++) err |= ipu_blur_x(rx);
		if (ry) for (int i=0; i<n; i++) err |= ipu_blur_y(ry);
		if (v[4] != 1) err |= ipu_mul(v[4], v[4], v[4]);
		return err;
	});

	operator("dup", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = ipu_dup();
		if (v[0] != 1) err |= ipu_mul(v[0], v[0], v[0]);
		return err;
	});

	operator("mix-add", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = ipu_mix_add();
		if (v[0] != 1) err |= ipu_mul(v[0], v[0], v[0]);
		return err;
	});

	operator("level", ({
		{ "clamp",	1,	0,		1,		1,		1		},
		{ "from",	0,	MIN,	MAX,	1e-6,	1e-2	},
		{ "to",		1,	MIN,	MAX,	1e-6,	1e-2	},
		{ "clamp",	0,	0,		1,		1,		1		},
	}), {
		bool err = false;
		bool clamp[2] = { v[0], v[3] };
		if (clamp[0]) err |= ipu_clamp();
		err |= ipu_level(v[1], v[2]);
		if (clamp[1]) err |= ipu_clamp();
		return err;
	});

	operator("move", ({
		{ "x", 10, -0xFFFF, 0xFFFF, 1, 1 },
		{ "y", 10, -0xFFFF, 0xFFFF, 1, 1 },
	}), {
		return ipu_move(v[0], v[1]);
	});

	operator("scale", ({
		{ "x", 0.5, -0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "y", 0.5, -0xFFFF, 0xFFFF, 1e-6, 1e-1 },
	}), {
		return ipu_scale(v[0], v[1]);
	});

	operator("rotate", ({
		{ "angle", 45, -0xFFFF, 0xFFFF, 1e-6, 1e-1 },
	}), {
		return ipu_rotate(v[0]);
	});

	operator("transform", ({
		{ "origin x",	10,		-0xFFFF, 0xFFFF, 1, 1 },
		{ "origin y",	10,		-0xFFFF, 0xFFFF, 1, 1 },
		{ "x-axis' x",	4,		-0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "x-axis' y",	-0.2,	-0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "y-axis' x",	0.7,	-0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "y-axis' y",	-0.1,	-0xFFFF, 0xFFFF, 1e-6, 1e-1 },
	}), {
		return ipu_transform(v[0], v[1], v[2], v[3], v[4], v[5]);
	});

	operator("bump", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = false;
		if (v[0] != 1) err |= ipu_mul(v[0], v[0], v[0]);
		err |= ipu_bump();
		return err;
	});
}

