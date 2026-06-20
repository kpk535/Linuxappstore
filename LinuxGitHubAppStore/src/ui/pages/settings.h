#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox *box;
    GtkEntry *token_entry;
    GtkButton *test_btn;
    GtkLabel *status_label;
} PageSettings;

PageSettings *page_settings_new(void);

#endif
