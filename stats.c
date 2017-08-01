#include <curses.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <panel.h>
#include "stats.h"

static int ROW, COL;
static int currrow = 1;

static void draw_progress_bar(uint x, uint y, uint width, uint total, uint v1, uint v2);

static void color_init()
{
    use_default_colors();
    init_pair(ZS_COLOR_WHITE, COLOR_WHITE, -1);
    init_pair(ZS_COLOR_RED, COLOR_RED, -1);
    init_pair(ZS_COLOR_GREEN, COLOR_GREEN, -1);
    init_pair(ZS_COLOR_CYAN, COLOR_CYAN, -1);
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

static void draw_worker_stats()
{
    zs_select_color(ZS_COLOR_WHITE);
    char worker_stats[] = "    Worker Stats: ";
    char tmp[32] = {0};
    mvaddnstr(currrow++, LEFT_ALIGN, worker_stats, sizeof(worker_stats));
    sprintf(tmp, "%d/%d/%d", 64, 30, 50);
    draw_progress_bar(sizeof(worker_stats) + 1, 3,
            COL - sizeof(worker_stats) - strlen(tmp) - 6, 64, 30, 50);
}

static void draw_task_worker_stats()
{
    zs_select_color(ZS_COLOR_WHITE);
    char tmp[32] = {0};
    char worker_stats[] = "TaskWorker Stats: ";
    sprintf(tmp, "%d/%d/%d", 64, 20, 40);
    mvaddnstr(currrow++, LEFT_ALIGN, worker_stats, sizeof(worker_stats));
    draw_progress_bar(sizeof(worker_stats) + 1, 4,
            COL - sizeof(worker_stats) - strlen(tmp) - 6, 64, 20, 40);
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

static void draw_base_info()
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
    draw_text_with_width(currrow, LEFT_ALIGN, "start time:     ", BASE_INFO_WIDTH, "2017-10-11 11:11:11");
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "last reload:  ", BASE_INFO_WIDTH, "2017-10-11 11:11:11");
    draw_text_with_width(currrow, LEFT_ALIGN, "connection num: ", BASE_INFO_WIDTH, "1111");
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "accept count: ", BASE_INFO_WIDTH, "123123");
    draw_text_with_width(currrow, LEFT_ALIGN, "close count:    ", BASE_INFO_WIDTH, "1111");
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "tasking num:  ", BASE_INFO_WIDTH, "100");

    draw_text_with_width(currrow, LEFT_ALIGN, "worker exit count(normal/abnomal): ", BASE_INFO_WIDTH, "%d/%d", 100, 200);
    line_step();
    draw_text_with_width(currrow++, LEFT_ALIGN + left_offset, "task worker exit count(normal/abnomal): ", BASE_INFO_WIDTH, "%d/%d", 50, 100);
}

static void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
{	int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}
static void win_show(WINDOW *win, char *label, int label_color,
        int startx, int starty, int height, int width)
{
	box(win, 0, 0);
	mvwaddch(win, 2, 0, ACS_LTEE);
	mvwhline(win, 2, 1, ACS_HLINE, width - 2);
	mvwaddch(win, 2, width - 1, ACS_RTEE);

	print_in_middle(win, 1, 0, width, label, COLOR_PAIR(label_color));
}

static void draw_worker_detail()
{
    currrow++;
    currrow++;

    WINDOW *worker_win;
    PANEL *worker_panel;
    char *title = "Worker Detail";

    int width, height;
    width = (COL - (LEFT_ALIGN * 2)) / 2 - 5;
    height = ROW - currrow - 1;
    worker_win = newwin(height, width, currrow, LEFT_ALIGN);
    worker_panel = new_panel(worker_win);
    win_show(worker_win, title, ZS_COLOR_GREEN, LEFT_ALIGN, currrow, height, width);
	update_panels();
	doupdate();
}

static void draw_task_worker_detail()
{
    WINDOW *worker_win;
    PANEL *worker_panel;
    char *title = "Task Worker Detail";

    int width, height;
    width = (COL - (LEFT_ALIGN * 2)) / 2 - 5;
    height = ROW - currrow - 1;
    worker_win = newwin(height, width, currrow, LEFT_ALIGN + width + 5);
    worker_panel = new_panel(worker_win);
    win_show(worker_win, title, ZS_COLOR_GREEN, LEFT_ALIGN + width + 5, currrow, height, width);
	update_panels();
	doupdate();
}

int main() {
    initscr();    /* initializes curses */
    start_color();
    noecho();
	cbreak();
    color_init();
    getmaxyx(stdscr, ROW, COL);
#if 1
    draw_title();
    draw_worker_stats();
    draw_task_worker_stats();
    draw_base_info();
#endif
    draw_worker_detail();
    draw_task_worker_detail();
    //refresh();
    getch();
    endwin();     /* cleanup curses */
    return 0;
}
