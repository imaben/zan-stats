#pragma once

#include <curses.h>
#include <panel.h>

typedef unsigned int uint;

#define WORKER_DETAIL_TH { \
        "WorkerID", \
        "StartTime", \
        "TotalRequest", \
        "Request", \
        "Status"\
}
#define WORKER_DETAIL_TH_NUM 5

enum zs_colors {
    ZS_COLOR_WHITE,
    ZS_COLOR_RED,
    ZS_COLOR_GREEN,
    ZS_COLOR_CYAN,
    ZS_COLOR_BLACK_GREEN,
    ZS_COLOR_BLACK_CYAN
};

typedef struct {
    uint x;
    uint y;
} zs_point;

struct worker_detail_item {
    int worker_id;
    char start_time[32];
    int total_request;
    int request;
    char status[8];
};

typedef struct {
    WINDOW *win;
    PANEL *panel;
    int width;
    int height;
    int x;
    int y;
    int total_worker;

    struct worker_detail_item *item;
    int th_width[WORKER_DETAIL_TH_NUM];

    int offset;
    int cursor;
} zs_worker_detail;

#define zs_select_color(color) attrset(COLOR_PAIR(color))
#define zs_bold_on() attron(A_BOLD)
#define zs_bold_off() attroff(A_BOLD)

#define BASE_INFO_WIDTH 60
#define LEFT_ALIGN 3

