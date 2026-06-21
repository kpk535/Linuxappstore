#ifndef SYSINFO_PAGE_H
#define SYSINFO_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox         *box;
    /* dynamic (updated on refresh) */
    GtkProgressBar *ram_bar;
    GtkLabel       *ram_pct;
    GtkLabel       *ram_det;
    GtkProgressBar *disk_bar;
    GtkLabel       *disk_pct;
    GtkLabel       *disk_det;
    GtkLabel       *uptime_val;
} PageSysinfo;

PageSysinfo *page_sysinfo_new(void);
void         page_sysinfo_refresh(PageSysinfo *page);

#endif
