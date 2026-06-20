#ifndef PAGE_LIBRARY_H
#define PAGE_LIBRARY_H

#include <gtk/gtk.h>
#include "../window.h"

GtkWidget* page_library_create(AppWindow *win);
void page_library_refresh(GtkWidget *page, AppWindow *win);

#endif
