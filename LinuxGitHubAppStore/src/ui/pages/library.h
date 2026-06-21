#ifndef LIBRARY_PAGE_H
#define LIBRARY_PAGE_H

#include <gtk/gtk.h>

#define LIBRARY_N_FILTERS 5

typedef struct {
    GtkBox     *box;
    GtkListBox *apps_list;
    GtkLabel   *hint;
    GtkLabel   *count_label;
    const char *active_filter;           /* points to a static string */
    GtkButton  *filter_btns[LIBRARY_N_FILTERS];
} PageLibrary;

PageLibrary *page_library_new(void);
void page_library_refresh(PageLibrary *page);

#endif
