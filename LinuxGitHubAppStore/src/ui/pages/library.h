#ifndef LIBRARY_PAGE_H
#define LIBRARY_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox *box;
} PageLibrary;

PageLibrary *page_library_new(void);

#endif
