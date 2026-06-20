#ifndef PAGE_PROFILE_H
#define PAGE_PROFILE_H

#include <gtk/gtk.h>
#include "../window.h"

GtkWidget* page_profile_create(AppWindow *win);
void page_profile_refresh(GtkWidget *page, AppWindow *win);

#endif
