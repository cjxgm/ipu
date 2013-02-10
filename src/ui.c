// vim: noet ts=4 sw=4 sts=0
#include <Elementary.h>
#include <Imlib2.h>
#include "util.h"


// simpler callback add
#define $$$($object, $event, $callback, $arg) \
	evas_object_smart_callback_add($object, $event, \
			(void *)($callback), (void *)($arg))


static EAPI_MAIN int elm_main(int argc, char * argv[]);


void ui_run()
{
	ELM_MAIN()
	main(0, NULL);
}

static EAPI_MAIN int elm_main(int argc, char * argv[])
{
	//------------------- main window
	$_(win, elm_win_util_standard_add("ipu", "Image Proc Unit"));
	evas_object_resize(win, 800, 600);
	$$$(win, "delete,request", &elm_exit, NULL);

	//------------------- main vbox
	$_(box, elm_box_add(win));
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, box);
	evas_object_show(box);

	//------------------- toolbar
	$_(toolbar, elm_toolbar_add(win));
	evas_object_size_hint_align_set(toolbar, EVAS_HINT_FILL, 0);
	elm_box_pack_end(box, toolbar);
	evas_object_show(toolbar);

	// toolbar items
	elm_toolbar_item_append(toolbar, "document-new", "New", NULL, NULL);
	elm_toolbar_item_append(toolbar, "document-open", "Open", NULL, NULL);
	elm_toolbar_item_append(toolbar, "document-save", "Save", NULL, NULL);

	//------------------- content hbox
	$_(content, elm_box_add(win));
	elm_box_horizontal_set(content, EINA_TRUE);
	evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(content, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(box, content);
	evas_object_show(content);

	//------------------- operators
	// frame
	$_(ops_frame, elm_frame_add(win));
	elm_object_text_set(ops_frame, "Operators");
	evas_object_size_hint_weight_set(ops_frame, 0.5, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(ops_frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, ops_frame);
	evas_object_show(ops_frame);

	// list
	$_(ops, elm_list_add(win));
	evas_object_size_hint_weight_set(ops, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(ops, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(ops_frame, ops);
	evas_object_show(ops);

	// list items
	elm_list_item_append(ops, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(ops, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(ops, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(ops, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(ops, "Hi!", NULL, NULL, NULL, NULL);

	//------------------- properties
	// frame
	$_(props_frame, elm_frame_add(win));
	elm_object_text_set(props_frame, "Properties");
	evas_object_size_hint_weight_set(props_frame, 1, 1);
	evas_object_size_hint_align_set(props_frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, props_frame);
	evas_object_show(props_frame);

	// scroller
	$_(props, elm_scroller_add(win));
	evas_object_size_hint_weight_set(props, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(props, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(props_frame, props);
	evas_object_show(props);

	//------------------- image stack
	// frame
	$_(stack_frame, elm_frame_add(win));
	elm_object_text_set(stack_frame, "Image Stack");
	evas_object_size_hint_weight_set(stack_frame, 0.5, 1);
	evas_object_size_hint_align_set(stack_frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, stack_frame);
	evas_object_show(stack_frame);

	// list
	$_(stack, elm_list_add(win));
	evas_object_size_hint_weight_set(stack, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(stack, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(stack_frame, stack);
	evas_object_show(stack);

	// list items
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);
	elm_list_item_append(stack, "Hi!", NULL, NULL, NULL, NULL);

	//------------------- done!
	evas_object_show(win);

	elm_run();
	elm_shutdown();
	return 0;
}

