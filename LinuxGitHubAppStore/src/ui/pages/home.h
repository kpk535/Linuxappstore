#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox *box;
    GtkSearchEntry *search_entry;
    GtkListBox *results_list;
} PageHome;

PageHome *page_home_new(void);

#endif
