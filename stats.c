/*
  +----------------------------------------------------------------------+
  | Zan-Stats                                                            |
  +----------------------------------------------------------------------+
  | Copyright (c) 2017-2017 maben<https://github.com/imaben>             |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.0 of the Apache license,    |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.apache.org/licenses/LICENSE-2.0.html                      |
  | If you did not receive a copy of the Apache2.0 license and are unable|
  | to obtain it through the world-wide-web, please send a note to       |
  | www.maben@foxmail.com so we can mail you a copy immediately.         |
  +----------------------------------------------------------------------+
  | Author: maben <www.maben@foxmail.com>                                |
  +----------------------------------------------------------------------+
*/

#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <inttypes.h>
#include <curl/curl.h>
#include "stats.h"
#include "smart_str.h"
#include "cJSON.h"

static int ROW, COL;
static int currrow         = 1;
static int update_interval = 1;
static char *request_url   = NULL;
static int win_inited      = 0;
static CURL *curl;

static void draw_progress_bar(uint x, uint y, uint width, uint total, uint v1, uint v2);
static zs_worker_detail *current_detail = NULL;
static zs_worker_detail *worker_details[2] = {0};

#define fatal(fmt, ...) do {             \
    if (win_inited) endwin();                            \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    exit(1);                             \
} while (0)


static void color_init()
{
    use_default_colors();
    init_pair(ZS_COLOR_WHITE,       COLOR_WHITE, -1);
    init_pair(ZS_COLOR_RED,         COLOR_RED,   -1);
    init_pair(ZS_COLOR_GREEN,       COLOR_GREEN, -1);
    init_pair(ZS_COLOR_CYAN,        COLOR_CYAN,  -1);
    init_pair(ZS_COLOR_BLACK_GREEN, 0,           COLOR_GREEN);
    init_pair(ZS_COLOR_BLACK_CYAN,  0,           COLOR_CYAN);
}

static void draw_progress_bar(uint x, uint y, uint width, uint total, uint v1, uint v2)
{
    assert(v1 <= total);
    assert(v2 <= total);
    assert(v1 <= v2);

    float fv1 = v1, fv2 = v2;

    fv1 = ceil((fv1 / total) * width);
    fv2 = ceil((fv2 / total) * width);

    uint i;
    zs_select_color(ZS_COLOR_WHITE);
    zs_bold_on();
    mvaddch(y, x++, '[');
    zs_bold_off();
    for (i = 0; i < width; i++) {
        if (i <= fv1) {
            zs_select_color(ZS_COLOR_GREEN);
        } else {
            zs_select_color(ZS_COLOR_RED);
        }
        if (i < fv2) {
            mvaddch(y, x + i, '|');
        }
    }
    zs_select_color(ZS_COLOR_WHITE);
    zs_bold_on();
    mvaddch(y, x + i, ']');
    zs_bold_off();
    mvprintw(y, x + i + 2, "%d/%d/%d", v1, v2, total);
}

static void draw_worker_stats(int total, int active, int max)
{
    zs_select_color(ZS_COLOR_WHITE);
    char worker_stats[] = "    Worker Stats: ";
    char tmp[32] = {0};
    mvaddnstr(currrow++, LEFT_ALIGN, worker_stats, sizeof(worker_stats));
    sprintf(tmp, "%d/%d/%d", total, active, max);
    draw_progress_bar(sizeof(worker_stats) + 1, 3,
            COL - sizeof(worker_stats) - strlen(tmp) - 7, total, active, max);
}

static void draw_task_worker_stats(int total, int active, int max)
{
    zs_select_color(ZS_COLOR_WHITE);
    char tmp[32] = {0};
    char worker_stats[] = "TaskWorker Stats: ";
    sprintf(tmp, "%d/%d/%d", total, active, max);
    mvaddnstr(currrow++, LEFT_ALIGN, worker_stats, sizeof(worker_stats));
    draw_progress_bar(sizeof(worker_stats) + 1, 4,
            COL - sizeof(worker_stats) - strlen(tmp) - 7, total, active, max);
    currrow++;
}

static void draw_title()
{
    zs_bold_on();
    char *title = "Zan stats [zanphp.io]";
    mvprintw(currrow, (COL-strlen(title))/2, "%s", title);
    zs_bold_off();
    currrow += 2;
}

static void draw_text_with_width(int y, int x, char *label, int total_width, char *format, ...)
{
    static char buff[1024] = {0};
    int pad, len;
    va_list al;
    mvprintw(y, x, label);

    memset(buff, 0, 1024);
    va_start(al, format);
    len = vsprintf(buff, format, al);
    va_end(al);

    pad = total_width - strlen(label) - len;
    if (pad > 0) {
        memset(buff + len, ' ', pad);
    }

    zs_bold_on();
    len = printw(buff);
    zs_bold_off();
}

static void draw_base_info(zs_base_info *base)
{
    int left_offset = 0;
#define line_step() do {\
    if (COL < (BASE_INFO_WIDTH * 2)) { \
        currrow++; \
        left_offset = 0; \
    } else {\
        left_offset = BASE_INFO_WIDTH; \
    } \
} while (0)
    draw_text_with_width(currrow, LEFT_ALIGN, "start time:     ", BASE_INFO_WIDTH, "%s", base->start_time);
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "last reload:  ", BASE_INFO_WIDTH, base->last_reload);
    draw_text_with_width(currrow, LEFT_ALIGN, "connection num: ", BASE_INFO_WIDTH, "%d", base->connection_num);
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "accept count: ", BASE_INFO_WIDTH, "%d", base->accept_count);
    draw_text_with_width(currrow, LEFT_ALIGN, "close count:    ", BASE_INFO_WIDTH, "%d", base->close_count);
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "tasking num:  ", BASE_INFO_WIDTH, "%d", base->tasking_num);

    draw_text_with_width(currrow, LEFT_ALIGN, "worker exit count(normal/abnomal): ", BASE_INFO_WIDTH, "%d/%d",
            base->worker_normal_exit, base->worker_abnormal_exit);
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "task worker exit count(normal/abnomal): ", BASE_INFO_WIDTH, "%d/%d",
            base->task_worker_normal_exit, base->task_worker_abnormal_exit);
}

static zs_worker_detail *worker_detail_new(char *title, int width, int height, int x, int y, int total_worker)
{
    zs_worker_detail *detail = malloc(sizeof(zs_worker_detail));
    memset(detail, 0, sizeof(zs_worker_detail));

    detail->width = width;
    detail->height = height;
    detail->x = x;
    detail->y = y;

    detail->total_worker = total_worker;
    detail->item = calloc(sizeof(struct worker_detail_item), total_worker);

    detail->win = newwin(height, width, y, x);
    detail->panel = new_panel(detail->win);

    keypad(detail->win, TRUE);
    // show
    box(detail->win, 0, 0);
    mvwaddch(detail->win, 2, 0, ACS_LTEE);
    mvwhline(detail->win, 2, 1, ACS_HLINE, width - 2);
    mvwaddch(detail->win, 2, width - 1, ACS_RTEE);

    // print title
    int length, tx;
    length = strlen(title);
    tx = (int)(width - length)/ 2;
    wattron(detail->win, COLOR_PAIR(ZS_COLOR_GREEN));
    mvwprintw(detail->win, 1, tx, "%s", title);
    wattroff(detail->win, COLOR_PAIR(ZS_COLOR_GREEN));
    refresh();

    // draw th
    int i, th_width = width - 2, pad;
    int col_width = floor(th_width / WORKER_DETAIL_TH_NUM);
    char *th[] = WORKER_DETAIL_TH;
    for (i = 0; i < WORKER_DETAIL_TH_NUM; i++) {
        detail->th_width[i] = col_width;
    }
    if ((pad = (col_width * WORKER_DETAIL_TH_NUM)) < th_width) {
        detail->th_width[WORKER_DETAIL_TH_NUM - 1] += (th_width - pad);
    }

    // to draw
    char tmp[512] = {0};
    char fmt[16] = {0};
    wmove(detail->win, 3, 1);
    wattron(detail->win, COLOR_PAIR(ZS_COLOR_BLACK_GREEN));
    zs_bold_on();
    for (i = 0; i < WORKER_DETAIL_TH_NUM; i++) {
        sprintf(fmt, "%%-%ds", detail->th_width[i]);
        sprintf(tmp, fmt, th[i]);
        wprintw(detail->win, "%s", tmp);
    }
    zs_bold_off();
    wattroff(detail->win, COLOR_PAIR(ZS_COLOR_BLACK_GREEN));

    return detail;
}

static int worker_detail_update(zs_worker_detail *detail, int offset, struct worker_detail_item *item)
{
    if (offset > (detail->total_worker - 1)) {
        return -1;
    }

    struct worker_detail_item *old = &detail->item[offset];
    memcpy(old, item, sizeof(struct worker_detail_item));

    return 0;
}

static void worker_detail_refresh(zs_worker_detail *detail)
{
    int i, j, pad;
    char tmp[512] = {0};
    char fmt[16] = {0};

    for (i = detail->offset, j = 0; i < detail->total_worker; i++, j++) {
        wmove(detail->win, 4 + j, 1);
        if (current_detail == detail && detail->cursor == j) {
            wattron(detail->win, COLOR_PAIR(ZS_COLOR_BLACK_CYAN));
        }
        // worker_id
        sprintf(fmt, "%%-%dd", detail->th_width[0] - 1);
        sprintf(tmp, fmt, detail->item[i].worker_id);
        wprintw(detail->win, " %s", tmp);

        // start_time
        sprintf(fmt, "%%-%ds", detail->th_width[1]);
        sprintf(tmp, fmt, detail->item[i].start_time);
        wprintw(detail->win, "%s", tmp);

        // total request
        sprintf(fmt, "%%-%dd", detail->th_width[2]);
        sprintf(tmp, fmt, detail->item[i].total_request);
        wprintw(detail->win, "%s", tmp);

        // request
        sprintf(fmt, "%%-%dd", detail->th_width[3]);
        sprintf(tmp, fmt, detail->item[i].request);
        wprintw(detail->win, "%s", tmp);

        // status
        sprintf(fmt, "%%-%ds", detail->th_width[4]);
        sprintf(tmp, fmt, detail->item[i].status);
        wprintw(detail->win, "%s", tmp);

        if (current_detail == detail && detail->cursor == j) {
            wattroff(detail->win, COLOR_PAIR(ZS_COLOR_BLACK_CYAN));
        }

        if (j > (detail->height - 7)) {
            break;
        }
    }
    wrefresh(detail->win);
}

static void worker_detail_free(zs_worker_detail* d)
{
    if (d->panel) {
        del_panel(d->panel);
    }
    if (d->win) {
        delwin(d->win);
    }
    if (d->item) {
        free(d->item);
    }
    free(d);
}

static void draw_worker_detail(int worker_num)
{
    currrow++;
    currrow++;

    int width, height, i;
    width = (COL - (LEFT_ALIGN * 2)) / 2 - 5;
    height = ROW - currrow - 1;

    zs_worker_detail *worker_detail = worker_detail_new("Worker Detail",
            width, height, LEFT_ALIGN, currrow, worker_num);
    worker_details[0] = worker_detail;
    current_detail = worker_detail;

    refresh();
    update_panels();
    doupdate();
}

static void draw_task_worker_detail(int worker_num)
{
    int width, height, i;
    width = (COL - (LEFT_ALIGN * 2)) / 2 - 5;
    height = ROW - currrow - 1;
    zs_worker_detail *worker_detail = worker_detail_new("Task Worker Detail",
            width, height, LEFT_ALIGN + width + 11, currrow, worker_num);
    worker_details[1] = worker_detail;

    refresh();
    update_panels();
    doupdate();
}

static void key_event_handler(int key)
{
    if (key == KEY_DOWN) {
        if (current_detail->cursor > (current_detail->height - 7)) {
            if ((current_detail->offset + current_detail->cursor + 1) < current_detail->total_worker) {
                current_detail->offset++;
            }
        } else {
            if ((current_detail->cursor + 1)< current_detail->total_worker) {
                current_detail->cursor++;
            }
        }
    } else if (key == KEY_UP) {
        if (current_detail->cursor == 0) {
            if (current_detail->offset > 0) {
                current_detail->offset--;
            }
        } else {
            current_detail->cursor--;
        }
    } else if (key == KEY_LEFT) {
        if (current_detail == worker_details[1]) {
            current_detail = worker_details[0];
            worker_detail_refresh(worker_details[1]);
        }
    } else if (key == KEY_RIGHT) {
        if (current_detail == worker_details[0] && worker_details[1]->total_worker > 0) {
            current_detail = worker_details[1];
            worker_detail_refresh(worker_details[0]);
        }
    }
    worker_detail_refresh(current_detail);
}

static void usage()
{
    const char *usage = "_____              _____ __        __\n"
        "/__  /  ____ _____ / ___// /_____ _/ /______\n"
        "  / /  / __ `/ __ \\\\__ \\/ __/ __ `/ __/ ___/\n"
        " / /__/ /_/ / / / /__/ / /_/ /_/ / /_(__  )\n"
        "/____/\\__,_/_/ /_/____/\\__/\\__,_/\\__/____/\n\n\n"
        "zan-stats [options] <url>\n\n"
        "options:\n"
        " -n <secs> seconds to wait between updates\n"
        " -h        show helper\n"
        " -v        show version\n\n";
    fprintf(stdout, "%s", usage);
}

static int parse_options(int argc, char **argv)
{
    int ch, c = 0;
    while ((ch = getopt(argc, argv, "n:hv")) != -1) {
        switch (ch) {
            case 'n':
                update_interval = atoi(optarg);
                if (update_interval <= 0) {
                    usage();
                    return -1;
                }
                c += 2;
                break;
            case 'h':
                usage();
                return -1;
            case 'v':
                fprintf(stdout, "Zan Stats " ZS_VERSION);
                return -1;
            case '?':
                return -1;
        }
    }

    if (argc < (2 + c)) {
        usage();
        return -1;
    }
    request_url = strdup(argv[argc - 1]);

    return 0;
}

size_t curl_write(void *ptr, size_t size, size_t count, void *stream)
{
    smart_str_appendl((smart_str *)stream, (char *)ptr, size * count);
    smart_str_0((smart_str*)stream);
    return size * count;
}

static void curl_init()
{
    CURLcode return_code;
    return_code = curl_global_init(CURL_GLOBAL_ALL);
    if (CURLE_OK != return_code)
    {
        fatal("init libcurl failed.\n");
    }

    curl = curl_easy_init();
    if (!curl) {
        fatal("init curl failed.\n");
    }
    curl_easy_setopt(curl, CURLOPT_URL, request_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
}

static void curl_cleanup()
{
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

static int unix2time(int ut, char *dst, int len)
{
    time_t t = ut;
    struct tm *p;
    p = gmtime(&t);
    return strftime(dst, len, "%Y-%m-%d %H:%M:%S", p);
}

static int format_start_time(int ut, char *dst, int len)
{
    time_t now = time(NULL);
    unsigned long long total_seconds = now - ut;
    if (total_seconds <= 0) {
        strncpy(dst, "0:00:00", len);
        return 0;
    }
    unsigned long long hours = total_seconds / 3600;
    int minutes = (total_seconds / 60) % 60;
    int seconds = total_seconds % 60;
    if (hours >= 100) {
        snprintf(dst, len, "%7lluh", hours);
    } else {
        if (hours) {
            snprintf(dst, len, "%2lluh%02d:%02d", hours, minutes, seconds);
        } else {
            snprintf(dst, len, "%2d:%02d.%02d ", hours, minutes, seconds);
        }
    }
    return 0;
}

static int check_stats_data(cJSON *root)
{
    if (!root) {
        return 0;
    }
    return (int)cJSON_HasObjectItem(root, "total_worker") &&
        cJSON_HasObjectItem(root, "active_worker") &&
        cJSON_HasObjectItem(root, "max_active_worker") &&
        cJSON_HasObjectItem(root, "total_task_worker") &&
        cJSON_HasObjectItem(root, "active_task_worker") &&
        cJSON_HasObjectItem(root, "max_active_task_worker") &&
        cJSON_HasObjectItem(root, "start_time") &&
        cJSON_HasObjectItem(root, "last_reload") &&
        cJSON_HasObjectItem(root, "connection_num") &&
        cJSON_HasObjectItem(root, "accept_count") &&
        cJSON_HasObjectItem(root, "close_count") &&
        cJSON_HasObjectItem(root, "tasking_num") &&
        cJSON_HasObjectItem(root, "worker_normal_exit") &&
        cJSON_HasObjectItem(root, "worker_abnormal_exit") &&
        cJSON_HasObjectItem(root, "task_worker_normal_exit") &&
        cJSON_HasObjectItem(root, "task_worker_abnormal_exit") &&
        cJSON_HasObjectItem(root, "workers_detail");
}

static int refresh_all()
{
    assert(curl != NULL);

    CURLcode res;
    smart_str str = {0};
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        fatal("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        return -1;
    }

    cJSON *root = cJSON_Parse(str.c);
    if (!check_stats_data(root)) {
        fatal("Invalid stats data");
    }
    cJSON *total_worker      = cJSON_GetObjectItem(root, "total_worker");
    cJSON *active_worker     = cJSON_GetObjectItem(root, "active_worker");
    cJSON *max_active_worker = cJSON_GetObjectItem(root, "max_active_worker");
    draw_worker_stats(total_worker->valueint, active_worker->valueint, max_active_worker->valueint);

    cJSON *total_task_worker      = cJSON_GetObjectItem(root, "total_task_worker");
    cJSON *active_task_worker     = cJSON_GetObjectItem(root, "active_task_worker");
    cJSON *max_active_task_worker = cJSON_GetObjectItem(root, "max_active_task_worker");
    draw_task_worker_stats(total_task_worker->valueint, active_task_worker->valueint, max_active_task_worker->valueint);

    zs_base_info base;
    cJSON *start_time                = cJSON_GetObjectItem(root, "start_time");
    cJSON *last_reload               = cJSON_GetObjectItem(root, "last_reload");
    cJSON *connection_num            = cJSON_GetObjectItem(root, "connection_num");
    cJSON *accept_count              = cJSON_GetObjectItem(root, "accept_count");
    cJSON *close_count               = cJSON_GetObjectItem(root, "close_count");
    cJSON *tasking_num               = cJSON_GetObjectItem(root, "tasking_num");
    cJSON *worker_normal_exit        = cJSON_GetObjectItem(root, "worker_normal_exit");
    cJSON *worker_abnormal_exit      = cJSON_GetObjectItem(root, "worker_abnormal_exit");
    cJSON *task_worker_normal_exit   = cJSON_GetObjectItem(root, "task_worker_normal_exit");
    cJSON *task_worker_abnormal_exit = cJSON_GetObjectItem(root, "task_worker_abnormal_exit");

    if (!start_time->valuestring) {
        unix2time(start_time->valueint, base.start_time, sizeof(base.start_time));
    } else {
        strcpy(base.start_time, start_time->valuestring);
    }
    if (!last_reload->valuestring) {
        if (!last_reload->valueint) {
            strcpy(base.last_reload, "(never)");
        } else {
            unix2time(last_reload->valueint, base.last_reload, sizeof(base.last_reload));
        }
    } else {
        strcpy(base.last_reload, last_reload->valuestring);
    }
    base.connection_num            = connection_num->valueint;
    base.accept_count              = accept_count->valuedouble;
    base.close_count               = close_count->valuedouble;
    base.tasking_num               = tasking_num->valueint;
    base.worker_normal_exit        = worker_normal_exit->valueint;
    base.worker_abnormal_exit      = worker_abnormal_exit->valueint;
    base.task_worker_normal_exit   = task_worker_normal_exit->valueint;
    base.task_worker_abnormal_exit = task_worker_abnormal_exit->valueint;
    draw_base_info(&base);

    if (worker_details[0] == NULL) {
        draw_worker_detail(total_worker->valueint);
    }
    if (worker_details[1] == NULL) {
        draw_task_worker_detail(total_task_worker->valueint);
    }

    cJSON *worker, *workers_detail = cJSON_GetObjectItem(root, "workers_detail");
    int i, j = 0, k = 0, total;
    total = total_worker->valueint + total_task_worker->valueint;
    struct worker_detail_item item;
    cJSON *item_start_time, *item_total_request_count,
          *item_request_count, *item_status, *item_type;
    for (i = 0; i < total; i++) {
        worker                   = cJSON_GetArrayItem(workers_detail, i);
        item_start_time          = cJSON_GetObjectItem(worker, "start_time");
        item_total_request_count = cJSON_GetObjectItem(worker, "total_request_count");
        item_request_count       = cJSON_GetObjectItem(worker, "request_count");
        item_status              = cJSON_GetObjectItem(worker, "status");
        item_type                = cJSON_GetObjectItem(worker, "type");

        format_start_time(item_start_time->valueint, item.start_time, sizeof(item.start_time));
        item.total_request = item_total_request_count->valuedouble;
        item.request = item_request_count->valuedouble;
        strcpy(item.status, item_status->valuestring);
        if (strcasecmp("worker", item_type->valuestring) == 0) {
            item.worker_id = j;
            worker_detail_update(worker_details[0], j, &item);
            j++;
        } else if (strcasecmp("task_worker", item_type->valuestring) == 0) {
            item.worker_id = k;
            worker_detail_update(worker_details[1], k, &item);
            k++;
        }
    }
    worker_detail_refresh(worker_details[0]);
    worker_detail_refresh(worker_details[1]);
    refresh();
    cJSON_Delete(root);
    smart_str_free(&str);

    return 0;
}

static void alarm_handler()
{
    currrow = 3;
    refresh_all();
    signal(SIGALRM, alarm_handler);
    alarm(update_interval);
}

int main(int argc, char **argv)
{
    if (parse_options(argc, argv) < 0) {
        return 1;
    }

    // init curl
    curl_init();
    initscr();    /* initializes curses */
    start_color();
    noecho();
    cbreak();
    curs_set(0);
    color_init();
    getmaxyx(stdscr, ROW, COL);
    win_inited = 1;
    draw_title();
    alarm_handler();
    refresh();

    // main loop
    int key, quit = 0;
    for (;;) {
        key = wgetch(worker_details[1]->win);
        switch (key) {
            case 'Q':
            case 'q':
                quit = 1;
                break;
            case KEY_LEFT:
            case KEY_RIGHT:
            case KEY_UP:
            case KEY_DOWN:
                key_event_handler(key);
                break;
        }
        if (quit) {
            break;
        }
    }

    if (worker_details[0]) {
        worker_detail_free(worker_details[0]);
    }

    if (worker_details[1]) {
        worker_detail_free(worker_details[1]);
    }
    endwin();     /* cleanup curses */
    curl_cleanup();
    return 0;
}
