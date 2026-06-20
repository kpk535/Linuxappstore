#ifndef WINDOW_H
#define WINDOW_H

#include <gtk/gtk.h>

typedef struct {
    GtkApplicationWindow *window;
    GtkBox *main_box;
    GtkBox *sidebar;
    GtkStack *pages;
} AppWindow;

AppWindow *app_window_new(GtkApplication *app);
void app_window_show(AppWindow *win);
void app_window_switch_page(AppWindow *win, const char *page_name);

#endif
