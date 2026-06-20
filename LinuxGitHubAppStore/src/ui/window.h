#ifndef WINDOW_H
#define WINDOW_H

#include <gtk/gtk.h>
#include "../services/settings.h"

typedef enum {
    PAGE_HOME,
    PAGE_LIBRARY,
    PAGE_SETTINGS,
    PAGE_PROFILE,
    PAGE_COUNT
} PageType;

typedef struct {
    GtkApplicationWindow *window;
    GtkBox *main_box;
    GtkBox *sidebar;
    GtkStack *page_stack;
    GtkWidget *pages[PAGE_COUNT];
    AppSettings *settings;
    char *current_token;
} AppWindow;

AppWindow* app_window_new(GtkApplication *app);
void app_window_free(AppWindow *win);
void app_window_show_page(AppWindow *win, PageType page);

#endif
