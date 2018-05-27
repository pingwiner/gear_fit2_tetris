#include <tizen.h>
#include "tetrisgame.h"
#include <cairo.h>
#include <math.h>
#include <Evas_GL.h>
#include <cairo-evas-gl.h>
#include <game.h>
#include <time.h>
#include <stdlib.h>

typedef struct appdata {
	Evas_Coord width;
	Evas_Coord height;

	Evas_Object *win;
	Evas_Object *img;

	cairo_surface_t *surface;
	cairo_t *cairo;

	cairo_device_t *cairo_device;
	Evas_GL *evas_gl;
	Evas_GL_Config *evas_gl_config;
	Evas_GL_Surface *evas_gl_surface;
	Evas_GL_Context *evas_gl_context;
} appdata_s;

typedef struct COLOR_T {
	double r, g, b;
} Color;

Color palette[7];

void init_palette() {
	palette[0].r = 1;
	palette[0].g = 1;
	palette[0].b = 1;

	for (int i = 1; i < 7; i++) {
		palette[i].r = drand48();
		palette[i].g = drand48();
		palette[i].b = drand48();
	}
}

Color get_field_color(int x, int y) {
	return palette[field[x + 1][y + 1]];
}

void draw_text(appdata_s *ad) {
	char buf[64] = {0};
	snprintf(buf, sizeof(buf), "score: %d", score);

	cairo_text_extents_t extents;
	double x, y;

	/* Set text as green color with opacity 0.8 value */
	cairo_set_source_rgba(ad->cairo, 0.2, 1.0, 0.2, 0.8);
 
	/* Set text font type as "Sans" */
	cairo_select_font_face(ad->cairo, "Sans",
		CAIRO_FONT_SLANT_NORMAL,
		CAIRO_FONT_WEIGHT_NORMAL);
 
	/* Set text font size as 52 */
	cairo_set_font_size(ad->cairo, 35.0);
 
	/* Set text position as align center */
	cairo_text_extents(ad->cairo, buf, &extents);
	x = ad->width/2 - (extents.width/2 + extents.x_bearing);
	y = ad->height - 20 - (extents.height/2 + extents.y_bearing);
	cairo_move_to(ad->cairo, x, y);
 
	/* Show text */
	cairo_show_text(ad->cairo, buf);
}

void cairo_drawing(void *data, Evas_Object *obj)
{
	appdata_s *ad = data;
	int i, j;

	model_tick();

	/* clear background as white */
	cairo_set_source_rgba(ad->cairo, 0, 0, 0, 1);
	cairo_paint(ad->cairo);

	cairo_set_source_rgba(ad->cairo, 1, 1, 1, 1);
	//cairo_rectangle(ad->cairo, 25, 20, FIELD_SIZE_X * 11, FIELD_SIZE_Y * 11);
	//cairo_fill(ad->cairo);

	cairo_set_operator(ad->cairo, CAIRO_OPERATOR_OVER);
	for(i = 0; i < FIELD_SIZE_Y; i++)
	{
		for(j = 0; j < FIELD_SIZE_X; j++) {
			cairo_line_cap_t line_cap_style = CAIRO_LINE_CAP_BUTT;
			cairo_line_join_t line_join_style = CAIRO_LINE_JOIN_MITER;

			double dash[] = {1.0, 1.0};

			cairo_set_dash(ad->cairo, dash, 2, 0);
			cairo_set_line_width(ad->cairo, 1);
			cairo_set_line_join(ad->cairo, line_join_style);
			cairo_set_line_cap(ad->cairo, line_cap_style);

			Color c  = get_field_color(i, j);
			cairo_set_source_rgba(ad->cairo, c.r, c.g, c.b, 1);

			float x = j * 19;
			float y = i * 19;
			cairo_rectangle(ad->cairo, x + 4, y, 18, 18);
			cairo_fill(ad->cairo);
		}
	}
	draw_text(ad);
	cairo_surface_flush(ad->surface);
}

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;

	cairo_surface_destroy(ad->surface);
	cairo_destroy(ad->cairo);
	cairo_device_destroy(ad->cairo_device);

	evas_gl_surface_destroy(ad->evas_gl, ad->evas_gl_surface);
	evas_gl_context_destroy(ad->evas_gl, ad->evas_gl_context);
	evas_gl_config_free(ad->evas_gl_config);
	evas_gl_free(ad->evas_gl);

	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static void
win_resize_cb(void *data, Evas *e , Evas_Object *obj , void *event_info)
{
	appdata_s *ad = data;

	if(ad->evas_gl_surface)
	{
		cairo_surface_destroy(ad->surface);
		cairo_destroy(ad->cairo);
		cairo_device_destroy(ad->cairo_device);
		evas_gl_surface_destroy(ad->evas_gl, ad->evas_gl_surface);
		ad->evas_gl_surface = NULL;
	}

	evas_object_geometry_get(obj, NULL, NULL, &ad->width, &ad->height);
	evas_object_image_size_set(ad->img, ad->width, ad->height);
	evas_object_resize(ad->img, ad->width, ad->height);
	evas_object_show(ad->img);

	if(!ad->evas_gl_surface)
	{
		Evas_Native_Surface ns;
		ad->evas_gl_surface = evas_gl_surface_create(ad->evas_gl, ad->evas_gl_config, ad->width, ad->height);
		evas_gl_native_surface_get(ad->evas_gl, ad->evas_gl_surface, &ns);
		evas_object_image_native_surface_set(ad->img, &ns);
		evas_object_image_pixels_dirty_set (ad->img, EINA_TRUE);

		ad->cairo_device = (cairo_device_t *)cairo_evas_gl_device_create (ad->evas_gl, ad->evas_gl_context);
		cairo_gl_device_set_thread_aware(ad->cairo_device, 0);
		ad->surface = (cairo_surface_t *)cairo_gl_surface_create_for_evas_gl(ad->cairo_device, ad->evas_gl_surface, ad->evas_gl_config, ad->width, ad->height);
		ad->cairo = cairo_create (ad->surface);
	}
}

void mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info) {
	Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *) event_info;

	int x = ev->output.x;
	int y = ev->output.y;
	if (y < 100) {
		rotate();
	} else if (y > 300) {
		down();
	} else {
		if (x < 108) {
			move_left();
		} else {
			move_right();
		}
	} 
	dlog_print(DLOG_ERROR, LOG_TAG, "mouse down %d, %d", ev->output.x, ev->output.y);
}

static void cairo_evasgl_drawing(appdata_s *ad)
{
	/* Window */
	elm_config_accel_preference_set("opengl");
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);
	if (elm_win_wm_rotation_supported_get(ad->win))
	{
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}
	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);
	evas_object_event_callback_add(ad->win, EVAS_CALLBACK_RESIZE, win_resize_cb, ad);
	evas_object_show(ad->win);

	/* Image */
	ad->img = evas_object_image_filled_add(evas_object_evas_get(ad->win));
	evas_object_event_callback_add(ad->img, EVAS_CALLBACK_MOUSE_DOWN, mouse_down_cb, NULL);
	evas_object_show(ad->img);

	evas_object_geometry_get(ad->win, NULL, NULL, &ad->width, &ad->height);

	/* Init EVASGL */
	Evas_Native_Surface ns;
	ad->evas_gl = evas_gl_new(evas_object_evas_get(ad->img));
	ad->evas_gl_config = evas_gl_config_new();
	ad->evas_gl_config->color_format = EVAS_GL_RGBA_8888;
	ad->evas_gl_surface = evas_gl_surface_create(ad->evas_gl, ad->evas_gl_config, ad->width, ad->height);
	ad->evas_gl_context = evas_gl_context_create(ad->evas_gl, NULL);
	evas_gl_native_surface_get(ad->evas_gl, ad->evas_gl_surface, &ns);
	evas_object_image_native_surface_set(ad->img, &ns);
	evas_object_image_pixels_get_callback_set(ad->img, cairo_drawing, ad);

	/* cairo & cairo device create with evasgl */
	setenv("CAIRO_GL_COMPOSITOR", "msaa", 1);
	ad->cairo_device = (cairo_device_t *)cairo_evas_gl_device_create (ad->evas_gl, ad->evas_gl_context);
	cairo_gl_device_set_thread_aware(ad->cairo_device, 0);
	ad->surface = (cairo_surface_t *)cairo_gl_surface_create_for_evas_gl(ad->cairo_device, ad->evas_gl_surface, ad->evas_gl_config, ad->width, ad->height);
	ad->cairo = cairo_create (ad->surface);
}

static Eina_Bool
_animate_cb(void *data)
{
	Evas_Object *obj = (Evas_Object *)data;
	evas_object_image_pixels_dirty_set(obj, EINA_TRUE);
	return EINA_TRUE;
}

static bool
app_create(void *data)
{
	appdata_s *ad = data;
	model_init();
	init_palette();
	cairo_evasgl_drawing(ad);

	ecore_animator_frametime_set(0.125);
	ecore_animator_add(_animate_cb, (void *)ad->img);

	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	srand(time(NULL));
	srand48(time(NULL));
	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
