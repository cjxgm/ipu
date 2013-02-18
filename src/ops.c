
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
		{ "r", 0, MIN, MAX, 1e-6, 1e-1 },
		{ "g", 0, MIN, MAX, 1e-6, 1e-1 },
		{ "b", 0, MIN, MAX, 1e-6, 1e-1 },
	}), {
		return ipu_color(v[0], v[1], v[2]);
	});

	operator("perlin-noise", ({ 
		{ "r", 1, MIN, MAX, 1e-6, 1e-1 },
		{ "g", 1, MIN, MAX, 1e-6, 1e-1 },
		{ "b", 1, MIN, MAX, 1e-6, 1e-1 },
		{ "origin x", 0, MIN, MAX, 1e-2, 1 },
		{ "origin y", 0, MIN, MAX, 1e-2, 1 },
		{ "scale x", 10, MIN, MAX, 1e-2, 0.5 },
		{ "scale y", 10, MIN, MAX, 1e-2, 0.5 },
		{ "persistence", 0.5, 0, 1, 1e-3, 0.125 },
		{ "recursion", 4, 1, 32, 1, 1 },
	}), {
		return ipu_pnoise(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);
	});

	operator("pixel", ({
		{ "r",		1,	MIN,	MAX,		1e-6,	1e-1	},
		{ "g",		1,	MIN,	MAX,		1e-6,	1e-1	},
		{ "b",		1,	MIN,	MAX,		1e-6,	1e-1	},
		{ "amount",	80,	0,		0xFFFF,		1,		1		},
		{ "seed",	0,	0,		0xFFFFFFFF,	1,		1		},
	}), {
		return ipu_pixel(v[0], v[1], v[2], v[3], v[4]);
	});

	operator("circle", ({
		{ "r",		1,		MIN,	MAX,		1e-6,	1e-1	},
		{ "g",		1,		MIN,	MAX,		1e-6,	1e-1	},
		{ "b",		1,		MIN,	MAX,		1e-6,	1e-1	},
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
		if (v[0] != 1) err |= ipu_mul(v[0]);
		if (rx) for (int i=0; i<n; i++) err |= ipu_blur_x(rx);
		if (ry) for (int i=0; i<n; i++) err |= ipu_blur_y(ry);
		if (v[4] != 1) err |= ipu_mul(v[4]);
		return err;
	});

	operator("dup", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = ipu_dup();
		if (v[0] != 1) err |= ipu_mul(v[0]);
		return err;
	});

	operator("mix-add", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = ipu_mix_add();
		if (v[0] != 1) err |= ipu_mul(v[0]);
		return err;
	});

	operator("mix-sub", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = ipu_mix_sub();
		if (v[0] != 1) err |= ipu_mul(v[0]);
		return err;
	});

	operator("mix-mul", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = ipu_mix_mul();
		if (v[0] != 1) err |= ipu_mul(v[0]);
		return err;
	});

	operator("mix-div", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = ipu_mix_div();
		if (v[0] != 1) err |= ipu_mul(v[0]);
		return err;
	});

	operator("level", ({
		{ "clamp",	1,	0,		1,		1,		1		},
		{ "from",	0,	MIN,	MAX,	1e-6,	1e-1	},
		{ "to",		1,	MIN,	MAX,	1e-6,	1e-1	},
		{ "clamp",	0,	0,		1,		1,		1		},
	}), {
		bool err = false;
		bool clamp[2] = { v[0], v[3] };
		if (clamp[0]) err |= ipu_clamp();
		err |= ipu_level(v[1], v[2]);
		if (clamp[1]) err |= ipu_clamp();
		return err;
	});

	operator("desaturate", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
		{ "clamp",	0,	0,		1,		1,		1		},
	}), {
		bool err = ipu_desaturate();
		bool clamp = v[1];
		if (v[0] != 1) err |= ipu_mul(v[0]);
		if (clamp) err |= ipu_clamp();
		return err;
	});

	operator("move", ({
		{ "x", 10, -0xFFFF, 0xFFFF, 1e-2, 0.5 },
		{ "y", 10, -0xFFFF, 0xFFFF, 1e-2, 0.5 },
		{ "super sampling",	0, 0, 1, 1, 1 },
	}), {
		return ipu_move(v[0], v[1], v[2]);
	});

	operator("scale", ({
		{ "x", 0.5, -0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "y", 0.5, -0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "super sampling",	0, 0, 1, 1, 1 },
	}), {
		return ipu_scale(v[0], v[1], v[2]);
	});

	operator("rotate", ({
		{ "angle", 45, -0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "super sampling",	0, 0, 1, 1, 1 },
	}), {
		return ipu_rotate(v[0], v[2]);
	});

	operator("transform", ({
		{ "origin x",	10,		-0xFFFF, 0xFFFF, 1, 1 },
		{ "origin y",	10,		-0xFFFF, 0xFFFF, 1, 1 },
		{ "x-axis' x",	4,		-0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "x-axis' y",	-0.2,	-0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "y-axis' x",	0.7,	-0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "y-axis' y",	-0.1,	-0xFFFF, 0xFFFF, 1e-6, 1e-1 },
		{ "super sampling",	0, 0, 1, 1, 1 },
	}), {
		return ipu_transform(v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
	});

	operator("bump", ({
		{ "amplify",	1,	0,	0xFFFF,	1e-6,	1e-1	},
	}), {
		bool err = false;
		if (v[0] != 1) err |= ipu_mul(v[0]);
		err |= ipu_bump();
		return err;
	});

	operator("displace", ({
		{ "size x",	20,	MIN,	MAX,	1e-2,	1 },
		{ "size y",	20,	MIN,	MAX,	1e-2,	1 },
	}), {
		return ipu_displace(v[0], v[1]);
	});
}

