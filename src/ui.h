// vim: noet ts=4 sw=4 sts=0
// GUI
#ifndef __IPU$UI__
#define __IPU$UI__

#include "util.h"


#define UI_MAX_OPERATORS	128

typedef struct PropInfo		// property information
{
	const char * name;		// property name
	float value;			// default value
	float min;
	float max;
	float round;			// how precise is it?
	float step;				// how much should I ++/-- when dragged?
}
PropInfo;

void ui_run();
void ui_register_operator(const char * name, int nprop,
		const PropInfo prop_infos[], bool pull(float v[]));


#endif

