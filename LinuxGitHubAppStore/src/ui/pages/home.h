#ifndef PAGE_HOME_H
#define PAGE_HOME_H

#include <gtk/gtk.h>
#include "../window.h"

GtkWidget* page_home_create(AppWindow *win);
void page_home_refresh(GtkWidget *page, AppWindow *win);

#endif
