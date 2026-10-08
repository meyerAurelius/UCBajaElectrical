#pragma once

typedef enum {
    SCREEN_NONE = -1,
    SCREEN_ENDURANCE = 0,
    SCREEN_DRIVER = 1
} screen_t;

extern volatile screen_t requested_screen;

