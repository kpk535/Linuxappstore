#ifndef PROFILE_PAGE_H
#define PROFILE_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox *box;
    GtkLabel *username_label;
    GtkLabel *name_label;
} PageProfile;

PageProfile *page_profile_new(void);

#endif
