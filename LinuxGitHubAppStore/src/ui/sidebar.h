#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <gtk/gtk.h>
#include "window.h"

typedef struct {
    AppWindow *app_window;
    GtkBox *container;
} SidebarContext;

GtkBox* sidebar_create(AppWindow *win);
void sidebar_set_nav_callback(GtkBox *sidebar, void (*callback)(PageType));

#endif
