#ifndef PAGE_SETTINGS_H
#define PAGE_SETTINGS_H

#include <gtk/gtk.h>
#include "../window.h"

GtkWidget* page_settings_create(AppWindow *win);
void page_settings_refresh(GtkWidget *page, AppWindow *win);

#endif
