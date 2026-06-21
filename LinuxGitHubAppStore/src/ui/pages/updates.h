#ifndef UPDATES_PAGE_H
#define UPDATES_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox     *box;
    GtkListBox *list;
    GtkLabel   *hint;
    GtkButton  *check_btn;
    GtkSpinner *spinner;
} PageUpdates;

PageUpdates *page_updates_new(void);

#endif
