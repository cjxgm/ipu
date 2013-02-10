// vim: noet ts=4 sw=4 sts=0
// GUI
#ifndef __IPU$UI__
#define __IPU$UI__

#include "util.h"


#define UI_MAX_OPERATORS	128

void ui_run();
void ui_register_operator(const char * name, int nprop,
		const char * prop_names[], bool poll(float props[]));


#endif

