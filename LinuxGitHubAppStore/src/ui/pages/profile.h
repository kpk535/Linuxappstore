#ifndef PROFILE_PAGE_H
#define PROFILE_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox    *box;
    GtkLabel  *login_label;
    GtkLabel  *name_label;
    GtkLabel  *bio_label;
    GtkLabel  *repos_val;
    GtkLabel  *followers_val;
    GtkLabel  *following_val;
    GtkButton *refresh_btn;
    GtkLabel  *status_label;
} PageProfile;

PageProfile *page_profile_new(void);
void page_profile_refresh(PageProfile *page);

#endif
