#ifndef LIBRARY_PAGE_H
#define LIBRARY_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox     *box;
    GtkListBox *apps_list;
    GtkLabel   *hint;
} PageLibrary;

PageLibrary *page_library_new(void);
void page_library_refresh(PageLibrary *page);

#endif
