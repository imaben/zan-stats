#pragma once

typedef unsigned int uint;

enum zs_colors {
    ZS_COLOR_WHITE,
    ZS_COLOR_RED,
    ZS_COLOR_GREEN,
    ZS_COLOR_CYAN
};

typedef struct {
    uint x;
    uint y;
} zs_point;

#define zs_select_color(color) attrset(COLOR_PAIR(color))
#define zs_bold_on() attron(A_BOLD)
#define zs_bold_off() attroff(A_BOLD)

#define BASE_INFO_WIDTH 60
#define LEFT_ALIGN 3

