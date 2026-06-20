#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox         *box;
    GtkStack       *stack;      /* "search" | "detail" */

    /* search sub-page */
    GtkSearchEntry *search_entry;
    GtkListBox     *results_list;
    GtkLabel       *results_hint;

    /* detail sub-page */
    GtkLabel       *detail_name;
    GtkLabel       *detail_meta;
    GtkLabel       *detail_desc;
    GtkListBox     *releases_list;
    GtkLabel       *releases_hint;
    char           *current_full_name;
    char           *current_app_name;
} PageHome;

PageHome *page_home_new(void);

#endif
