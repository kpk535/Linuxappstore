#ifndef SYSINFO_PAGE_H
#define SYSINFO_PAGE_H

#include <gtk/gtk.h>

typedef struct {
    GtkBox   *box;
    GtkLabel *os_label;
    GtkLabel *kernel_label;
    GtkLabel *cpu_label;
    GtkLabel *ram_label;
    GtkLabel *disk_label;
    GtkLabel *hostname_label;
} PageSysinfo;

PageSysinfo *page_sysinfo_new(void);

#endif
