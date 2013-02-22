// vim: noet ts=4 sw=4 sts=0
#include <Elementary.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ui.h"
#include "util.h"
#include "ops.h"
#include "ipu.h"


// simpler callback add
#define $$$$($object, $event, $callback, $arg) \
	evas_object_smart_callback_add($object, $event, \
			(void *)($callback), (void *)($arg))
#define $$$($object, $event, $callback, $arg) \
	evas_object_event_callback_add($object, $event, \
			(void *)($callback), (void *)($arg))


typedef struct Operator
{
	const char * name;
	int nprop;
	bool (*pull)(float v[]);
	const PropInfo * infos;
	Evas_Object * table;
	Evas_Object ** objs;
}
Operator;

static Operator ops[UI_MAX_OPERATORS];
static int ops_used;


static Evas_Object * win;
static Evas_Object * toolbar;
static Evas_Object * nodes;
static Evas_Object * props;
static Evas_Object * stack;
static Evas_Object * menu_node;
static Elm_Object_Item * menu_add;
static Elm_Object_Item * menu_delete;
static Evas_Object * props_current;


EAPI_MAIN int elm_main(int argc, char * argv[]);
static void execute_nodes();
static Evas_Object * limit_min_size(Evas_Object * o, int w, int h);
static void popup_message(const char * message);
static void popup_file_selector(const char * title, bool is_save,
								void done(const char * filename));
static void toolbar_no_selected();
static void document_new();
static void document_open(const char * fn);
static void document_save(const char * fn);
static Operator * op_find_by_name(const char * name);
static void op_add(Operator * op, float * values);

extern const char * initial_file_to_open;	// in main.c


void ui_run()
{
	ELM_MAIN()
	main(0, NULL);
}

EAPI_MAIN int elm_main(int argc, char * argv[])
{
	//------------------- main window
	win = elm_win_util_standard_add("ipu", "Image Proc Unit");
	evas_object_resize(win, 800, 600);
	$$$$(win, "delete,request", &elm_exit, NULL);

	//------------------- main vbox
	$_(box, elm_box_add(win));
	evas_object_size_hint_weight_set(box,
			EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, box);
	evas_object_show(box);

	//------------------- toolbar
	toolbar = elm_toolbar_add(win);
	evas_object_size_hint_align_set(toolbar, EVAS_HINT_FILL, 0);
	elm_box_pack_end(box, toolbar);
	evas_object_show(toolbar);

	// toolbar items
	elm_toolbar_item_append(toolbar, "document-new", "New",
		(void *)$(void, () {
			toolbar_no_selected();
			document_new();
		}), NULL);
	elm_toolbar_item_append(toolbar, "document-open", "Open",
		(void *)$(void, () {
			toolbar_no_selected();
			popup_file_selector("Open what?", false, &document_open);
		}), NULL);
	elm_toolbar_item_append(toolbar, "document-save", "Save",
		(void *)$(void, () {
			toolbar_no_selected();
			popup_file_selector("Save to?", true, &document_save);
		}), NULL);
	elm_toolbar_item_append(toolbar, "edit-delete", "Exit", (void *)&elm_exit, NULL);

	//------------------- content hbox
	$_(content, elm_box_add(win));
	elm_box_horizontal_set(content, EINA_TRUE);
	evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(content, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(box, content);
	evas_object_show(content);

	//------------------- nodes
	// frame
	$_(nodes_frame, elm_frame_add(win));
	elm_object_text_set(nodes_frame, "Nodes");
	evas_object_size_hint_weight_set(nodes_frame, 0.2, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(nodes_frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, nodes_frame);
	evas_object_show(nodes_frame);

	// list
	nodes = elm_list_add(win);
	evas_object_size_hint_weight_set(nodes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(nodes, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(nodes_frame, nodes);
	evas_object_show(nodes);

	// menu
	menu_node = elm_menu_add(win);
	// show menu when right clicked
	$$$(nodes, EVAS_CALLBACK_MOUSE_DOWN,
		$(void, (void * $1, void * $2, void * $3, Evas_Event_Mouse_Down * ev) {
			if (ev->button == 3) {
				// when no item selected, disable "Delete" item
				elm_object_item_disabled_set(menu_delete, !elm_list_selected_item_get(nodes));

				elm_menu_move(menu_node, ev->canvas.x, ev->canvas.y);
				evas_object_show(menu_node);
			}
		}), NULL);

	menu_add = elm_menu_item_add(menu_node, NULL, "document-new", "Add", NULL, NULL);

	menu_delete = elm_menu_item_add(menu_node, NULL,
		"edit-delete", "Delete", (void *)$(void, () {
			$_(item, elm_list_selected_item_get(nodes));
			$_(o, elm_menu_item_object_get(item));
			Operator * op = evas_object_data_get(o, "ipu:operator");

			elm_object_content_unset(props);
			evas_object_hide(op->table);
			props_current = NULL;

			free(evas_object_data_get(o, "ipu:v"));
			elm_object_item_del(item);

			execute_nodes();
		}), NULL);

	//------------------- properties
	// frame
	$_(props_frame, elm_frame_add(win));
	elm_object_text_set(props_frame, "Properties");
	evas_object_size_hint_weight_set(props_frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(props_frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, props_frame);
	evas_object_show(props_frame);

	// scroller
	props = elm_scroller_add(win);
	evas_object_size_hint_weight_set(props, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(props, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(props_frame, props);
	evas_object_show(props);

	//------------------- image stack
	// frame
	$_(stack_frame, elm_frame_add(win));
	elm_object_text_set(stack_frame, "Image Stack");
	evas_object_size_hint_weight_set(stack_frame, 0.75, 1);
	evas_object_size_hint_align_set(stack_frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_pack_end(content, stack_frame);
	evas_object_show(stack_frame);

	// scroller
	$_(stack_scroller, elm_scroller_add(win));
	evas_object_size_hint_weight_set(stack_scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_fill_set(stack_scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(stack_frame, stack_scroller);
	evas_object_show(stack_scroller);

	// table
	stack = elm_table_add(win);
	evas_object_size_hint_weight_set(stack, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_fill_set(stack, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_content_set(stack_scroller, stack);
	evas_object_show(stack);

	//------------------- prepare operators
	ops_register_operators();

	//------------------- done!
	evas_object_show(win);

	if (initial_file_to_open) document_open(initial_file_to_open);

	elm_run();
	elm_shutdown();
	return 0;
}

void ui_register_operator(const char * name, int nprop,
		const PropInfo prop_infos[], bool pull(float v[]))
{
	ops[ops_used].name  = name;
	ops[ops_used].nprop = nprop;
	ops[ops_used].pull  = pull;
	ops[ops_used].infos = prop_infos;
	ops[ops_used].objs  = new(void *, * nprop);

	int i;
	$_(table, elm_table_add(win));
	evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_fill_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
	for (i=0; i<nprop; i++) {
		$_(info, &prop_infos[i]);

		// label
		$_(label, elm_label_add(win));
		evas_object_size_hint_padding_set(label, 5, 0, 0, 0);
		elm_object_text_set(label, info->name);
		elm_table_pack(table, label, 0, i, 1, 1);
		evas_object_show(label);

		// spinner
		$_(spinner, elm_spinner_add(win));
		evas_object_size_hint_weight_set(spinner, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_fill_set(spinner, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_size_hint_padding_set(spinner, 5, 5, 5, 0);
		elm_spinner_min_max_set(spinner, info->min, info->max);
		elm_spinner_round_set(spinner, info->round);
		elm_spinner_step_set(spinner, info->step);
		elm_spinner_label_format_set(spinner, "%g");
		elm_table_pack(table, spinner, 1, i, 1, 1);
		evas_object_show(spinner);

		$$$$(spinner, "delay,changed", $(void, (int idx, Evas_Object * s) {
			$_(o, elm_menu_item_object_get(elm_list_selected_item_get(nodes)));

			// set property
			float * values = evas_object_data_get(o, "ipu:v");
			values[idx] = elm_spinner_value_get(s);

			execute_nodes();
		}), i);

		ops[ops_used].objs[i] = spinner;
	}
	ops[ops_used].table = table;

	elm_menu_item_add(menu_node, menu_add, NULL, name,
		(void *)$(void, (Operator * op) {
			op_add(op, NULL);
			elm_list_go(nodes);
			execute_nodes();
		}), &ops[ops_used]);

	ops_used++;
}

static void execute_nodes()
{
	// reset
	ipu_stack_clear();

	// execute nodes
	$_(item, elm_list_first_item_get(nodes));
	$_(item_selected, elm_list_selected_item_get(nodes));
	while (item) {
		$_(o, elm_menu_item_object_get(item));
		Operator * op  = evas_object_data_get(o, "ipu:operator");
		float * values = evas_object_data_get(o, "ipu:v");

		if (op->pull(values)) {
			if (item_selected == item)
				popup_message("An error occured when executing nodes.");
			else elm_list_item_selected_set(item, true);
			break;
		}

		if (item == item_selected) break;
		item = elm_list_item_next(item);
	}

	// show result in Image Stack
	elm_table_clear(stack, true);
	for (int i=ipu_stack_length()-1; i>=0; i--) {
		$_(image, elm_image_add(win));
		elm_image_resizable_set(image, 0, 0);
		evas_object_size_hint_min_set(image, 256, 256);
		evas_object_size_hint_padding_set(image, 10, 10, 10, 0);

		$$$(image, EVAS_CALLBACK_FREE,
			$(void, (void * $1, void * $2, Evas_Object * o) {
				ipu_ppm_free(evas_object_data_get(o, "ipu:ppm"));
			}), NULL);

		$$$$(image, "clicked", $(void, (void * $1, Evas_Object * o) {
			// "static"s can be safely used for later callbacks.
			static unsigned char * ppm;
			static size_t ppm_size;

			ppm = evas_object_data_get(o, "ipu:ppm");
			ppm_size = (size_t)evas_object_data_get(o, "ipu:ppm_size");

			popup_file_selector("Save PPM image to:", true,
				$(void, (const char * filename) {
					FILE * fp = fopen(filename, "w");
					if (!fp) popup_message("Cannot save image!");
					fwrite(ppm, ppm_size, 1, fp);
					fclose(fp);
				}));
		}), NULL);

		size_t ppm_size;
		$_(ppm, ipu_ppm_get(&ppm_size));
		elm_image_memfile_set(image, ppm, ppm_size, "ppm", NULL);
		evas_object_data_set(image, "ipu:ppm", ppm);
		evas_object_data_set(image, "ipu:ppm_size", (void *)ppm_size);

		elm_table_pack(stack, image, 0, i, 1, 1);
		evas_object_show(image);

		ipu_ignore();
	}
}

static Evas_Object * limit_min_size(Evas_Object * o, int w, int h)
{
	$_(table, elm_table_add(win));
	$_(rect, evas_object_rectangle_add(evas_object_evas_get(o)));
	evas_object_size_hint_min_set(rect, w, h);
	elm_table_pack(table, rect, 0, 0, 1, 1);
	elm_table_pack(table, o, 0, 0, 1, 1);
	return table;
}

static void popup_message(const char * message)
{
	$_(popup, elm_popup_add(win));
	elm_popup_orient_set(popup, ELM_POPUP_ORIENT_BOTTOM);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_part_text_set(popup, "title,text", message);
	evas_object_show(popup);

	$$$$(popup, "block,clicked", $(void, (void * $1, Evas_Object * o) {
		evas_object_del(o);
	}), NULL);
}

static void popup_file_selector(const char * title, bool is_save,
								void done(const char * filename))
{
	$_(popup, elm_popup_add(win));
	elm_popup_orient_set(popup, ELM_POPUP_ORIENT_TOP);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_part_text_set(popup, "title,text", title);
	$$$$(popup, "block,clicked", $(void, (void * $1, Evas_Object * o) {
		evas_object_del(o);
	}), NULL);

	$_(fs, elm_fileselector_add(popup));
	elm_fileselector_is_save_set(fs, is_save);
	elm_fileselector_expandable_set(fs, false);
	elm_fileselector_buttons_ok_cancel_set(fs, false);
	elm_fileselector_path_set(fs, getenv("PWD"));
	evas_object_size_hint_align_set(fs, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(fs);
	elm_object_content_set(popup, limit_min_size(fs, 400, 400));

	$_(btn, elm_button_add(popup));
	elm_object_text_set(btn, "OK");
	elm_object_part_content_set(popup, "button1", btn);

	// set data and bind event
	evas_object_data_set(popup, "cb:done", done);
	evas_object_data_set(popup, "cb:fs", fs);
	$$$$(btn, "clicked", $(void, (Evas_Object * o) {
		void (*done_cb)(const char * fn) = evas_object_data_get(o, "cb:done");
		Evas_Object * f = evas_object_data_get(o, "cb:fs");
		done_cb(elm_fileselector_selected_get(f));
		evas_object_del(o);
	}), popup);

	evas_object_show(popup);
}

static void toolbar_no_selected()
{
	$_(item, elm_toolbar_selected_item_get(toolbar));
	if (item) elm_toolbar_item_selected_set(item, false);
}

static void document_new()
{
	if (props_current) {
		elm_object_content_unset(props);
		evas_object_hide(props_current);
	}
	elm_table_clear(stack, true);

	Elm_Object_Item * item;
	while ((item = elm_list_last_item_get(nodes))) {
		$_(o, elm_menu_item_object_get(item));
		free(evas_object_data_get(o, "ipu:v"));
		elm_object_item_del(item);
	}
	elm_list_go(nodes);
}

static void document_open(const char * fn)
{
	if (!fn) {
		popup_message("So, what on earth are you fucking to open?!!");
		return;
	}

	FILE * fp = fopen(fn, "r");
	if (!fp) {
		popup_message("Cannot open that.");
		return;
	}

	document_new();

	char op_name[64];
	while (fscanf(fp, "%63s", op_name) != EOF) {
		$_(op, op_find_by_name(op_name));
		if (!op) {
			fclose(fp);
			popup_message("Invalid file!");
			return;
		}

		float * values = new(float, * op->nprop);
		for (int i=0; i<op->nprop; i++)
			fscanf(fp, "%g", &values[i]);
		op_add(op, values);
	}

	fclose(fp);
	popup_message("Opened!");

	elm_list_go(nodes);
	elm_list_item_bring_in(elm_list_last_item_get(nodes));
	execute_nodes();
}

static void document_save(const char * fn)
{
	if (!fn) {
		popup_message("So, why you clicked the fucking save?!!");
		return;
	}

	FILE * fp = fopen(fn, "w");
	if (!fp) {
		popup_message("Cannot save to there.");
		return;
	}

	$_(item, elm_list_first_item_get(nodes));
	while (item) {
		$_(o, elm_menu_item_object_get(item));
		Operator * op  = evas_object_data_get(o, "ipu:operator");
		float * values = evas_object_data_get(o, "ipu:v");

		fprintf(fp, "%s", op->name);
		for (int i=0; i<op->nprop; i++)
			fprintf(fp, " %g", values[i]);
		fprintf(fp, "\n");

		item = elm_list_item_next(item);
	}

	fclose(fp);
	popup_message("Saved!");
}

static Operator * op_find_by_name(const char * name)
{
	for (int i=0; i<ops_used; i++)
		if (!strcmp(name, ops[i].name))
			return &ops[i];
	return NULL;
}

// values can be NULL if you want to use default value's copy
static void op_add(Operator * op, float * values)
{
	void node_select_cb()
	{
		$_(o, elm_menu_item_object_get(elm_list_selected_item_get(nodes)));
		Operator * op = evas_object_data_get(o, "ipu:operator");

		// switch properties editing widgets to the selected's
		if (props_current) {
			elm_object_content_unset(props);
			evas_object_hide(props_current);
		}
		props_current = op->table;
		elm_object_content_set(props, props_current);

		// set properties
		float * values = evas_object_data_get(o, "ipu:v");
		for (int i=0; i<op->nprop; i++)
			elm_spinner_value_set(op->objs[i], values[i]);

		// then, show it
		evas_object_show(props_current);

		execute_nodes();
	};

	$_(o, elm_menu_item_object_get(elm_list_item_append(nodes,
			op->name, NULL, NULL, (void *)&node_select_cb, NULL)));

	// setup datas needed
	// operator
	evas_object_data_set(o, "ipu:operator", op);
	// values
	if (!values) {
		values = new(float, * op->nprop);
		for (int i=0; i<op->nprop; i++)
			values[i] = op->infos[i].value;
	}
	evas_object_data_set(o, "ipu:v", values);
}

