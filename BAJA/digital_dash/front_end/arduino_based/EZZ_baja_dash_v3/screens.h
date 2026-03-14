#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *wifi_settings;
    lv_obj_t *connectivity;
    lv_obj_t *connectivity_ch;
    lv_obj_t *wi_fi;
    lv_obj_t *wi_fi_ch;
    lv_obj_t *obj0;
    lv_obj_t *speed_readings;
    lv_obj_t *speed_needle;
    lv_obj_t *gradient_arc;
    lv_obj_t *value_in_speed;
    lv_obj_t *kmh;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_WIFI_SETTINGS = 2,
};

void create_screen_main();
void tick_screen_main();

void create_screen_wifi_settings();
void tick_screen_wifi_settings();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/