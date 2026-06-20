#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox         *box;
    GtkStack       *stack;           /* "search" | "detail" */

    /* search sub-page */
    GtkSearchEntry *search_entry;
    GtkSpinner     *search_spinner;
    GtkBox         *featured_box;
    GtkListBox     *results_list;
    GtkLabel       *results_hint;

    /* detail sub-page */
    GtkLabel       *detail_name;
    GtkLabel       *detail_meta;
    GtkLabel       *detail_desc;
    GtkSpinner     *rel_spinner;
    GtkListBox     *releases_list;
    GtkLabel       *releases_hint;
    char           *current_full_name;
    char           *current_app_name;
} PageHome;

PageHome *page_home_new(void);

#endif
