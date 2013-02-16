
#include <stdio.h>
#include "ipu.h"
#include "ui.h"

const char * initial_file_to_open;

int main(int argc, const char * argv[])
{
	if (argc == 2) initial_file_to_open = argv[1];
	ipu_init();
	ui_run();
	return 0;
}

