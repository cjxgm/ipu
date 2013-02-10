
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
}

