#include "sysinfo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

static char *read_os_field(const char *key) {
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) return NULL;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            fclose(f);
            char *val = strchr(line, '=');
            if (!val) return NULL;
            val++;
            size_t len = strlen(val);
            if (len && val[len-1] == '\n') val[--len] = '\0';
            if (len >= 2 && val[0] == '"' && val[len-1] == '"') {
                val[len-1] = '\0'; val++;
            }
            return strdup(val);
        }
    }
    fclose(f);
    return NULL;
}

static char *get_cpu_model(void) {
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) return strdup("Unknown CPU");
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "model name", 10) == 0 ||
            strncmp(line, "Hardware",    8) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                fclose(f);
                char *val = colon + 2;
                size_t len = strlen(val);
                if (len && val[len-1] == '\n') val[--len] = '\0';
                return strdup(val);
            }
        }
    }
    fclose(f);
    return strdup("Unknown CPU");
}

static void get_memory_mb(unsigned long *total, unsigned long *used, unsigned long *avail) {
    *total = 0; *used = 0; *avail = 0;
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    char line[256];
    unsigned long kb;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %lu kB", &kb) == 1)     *total = kb / 1024;
        if (sscanf(line, "MemAvailable: %lu kB", &kb) == 1) *avail = kb / 1024;
    }
    fclose(f);
    if (*total > *avail) *used = *total - *avail;
}

static GtkWidget *stat_card(const char *icon, const char *value, const char *label) {
    GtkBox *card = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
    gtk_widget_add_css_class(GTK_WIDGET(card), "stat-box");
    gtk_widget_set_hexpand(GTK_WIDGET(card), TRUE);

    char markup[80];
    snprintf(markup, sizeof(markup), "<span size='xx-large'>%s</span>", icon);
    GtkLabel *ico = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(ico, markup);
    gtk_label_set_xalign(ico, 0.5f);
    gtk_box_append(card, GTK_WIDGET(ico));

    GtkLabel *val = GTK_LABEL(gtk_label_new(value));
    gtk_widget_add_css_class(GTK_WIDGET(val), "stat-value");
    gtk_label_set_xalign(val, 0.5f);
    gtk_label_set_justify(val, GTK_JUSTIFY_CENTER);
    gtk_label_set_wrap(val, TRUE);
    gtk_box_append(card, GTK_WIDGET(val));

    GtkLabel *lbl = GTK_LABEL(gtk_label_new(label));
    gtk_widget_add_css_class(GTK_WIDGET(lbl), "stat-label");
    gtk_label_set_xalign(lbl, 0.5f);
    gtk_box_append(card, GTK_WIDGET(lbl));

    return GTK_WIDGET(card);
}

static GtkWidget *usage_bar(const char *label, const char *detail,
                             double fraction, const char *bar_css) {
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
    gtk_widget_add_css_class(GTK_WIDGET(box), "stat-box");
    gtk_widget_set_hexpand(GTK_WIDGET(box), TRUE);

    GtkBox *hdr = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4));
    GtkLabel *lbl = GTK_LABEL(gtk_label_new(label));
    gtk_widget_add_css_class(GTK_WIDGET(lbl), "stat-label");
    gtk_widget_set_hexpand(GTK_WIDGET(lbl), TRUE);
    gtk_label_set_xalign(lbl, 0.0f);
    gtk_box_append(hdr, GTK_WIDGET(lbl));

    GtkLabel *det = GTK_LABEL(gtk_label_new(detail));
    gtk_widget_add_css_class(GTK_WIDGET(det), "muted");
    gtk_box_append(hdr, GTK_WIDGET(det));
    gtk_box_append(box, GTK_WIDGET(hdr));

    GtkProgressBar *bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_widget_add_css_class(GTK_WIDGET(bar), bar_css);
    gtk_progress_bar_set_fraction(bar, fraction);
    gtk_box_append(box, GTK_WIDGET(bar));

    char pct_str[16];
    snprintf(pct_str, sizeof(pct_str), "%.0f%%", fraction * 100.0);
    GtkLabel *pct = GTK_LABEL(gtk_label_new(pct_str));
    gtk_widget_add_css_class(GTK_WIDGET(pct), "stat-value");
    gtk_label_set_xalign(pct, 0.5f);
    gtk_box_append(box, GTK_WIDGET(pct));

    return GTK_WIDGET(box);
}

PageSysinfo *page_sysinfo_new(void) {
    PageSysinfo *page = calloc(1, sizeof(PageSysinfo));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    GtkBox *inner = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 16));
    gtk_widget_set_margin_start(GTK_WIDGET(inner), 24);
    gtk_widget_set_margin_end(GTK_WIDGET(inner), 24);
    gtk_widget_set_margin_top(GTK_WIDGET(inner), 24);
    gtk_widget_set_margin_bottom(GTK_WIDGET(inner), 24);

    /* Title */
    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<span size='x-large'><b>System Info</b></span>");
    gtk_label_set_xalign(title, 0.0f);
    gtk_box_append(inner, GTK_WIDGET(title));

    GtkLabel *sub = GTK_LABEL(gtk_label_new("Hardware and operating system details for this machine."));
    gtk_widget_add_css_class(GTK_WIDGET(sub), "page-subtitle");
    gtk_label_set_xalign(sub, 0.0f);
    gtk_box_append(inner, GTK_WIDGET(sub));

    /* Collect data */
    struct utsname uts;
    uname(&uts);

    char hostname[256] = "unknown";
    gethostname(hostname, sizeof(hostname));

    char *os_pretty = read_os_field("PRETTY_NAME");
    char *cpu_model = get_cpu_model();

    unsigned long mem_total = 0, mem_used = 0, mem_avail = 0;
    get_memory_mb(&mem_total, &mem_used, &mem_avail);

    unsigned long disk_total_gb = 0, disk_free_gb = 0;
    struct statvfs vfs;
    if (statvfs("/", &vfs) == 0) {
        disk_total_gb = (unsigned long)((double)vfs.f_blocks * vfs.f_frsize / (1024.0*1024.0*1024.0));
        disk_free_gb  = (unsigned long)((double)vfs.f_bfree  * vfs.f_frsize / (1024.0*1024.0*1024.0));
    }
    unsigned long disk_used_gb = disk_total_gb > disk_free_gb
                                 ? disk_total_gb - disk_free_gb : 0;

    /* Row 1: Hostname + OS */
    GtkBox *r1 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_box_append(r1, stat_card("🖥", hostname, "Hostname"));
    gtk_box_append(r1, stat_card("🐧", os_pretty ?: "Linux", "Operating System"));
    gtk_box_append(inner, GTK_WIDGET(r1));

    /* Row 2: Kernel + CPU */
    GtkBox *r2 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_box_append(r2, stat_card("⚙", uts.release, "Kernel"));
    char cpu_short[128];
    snprintf(cpu_short, sizeof(cpu_short), "%.126s", cpu_model);
    gtk_box_append(r2, stat_card("🔧", cpu_short, "Processor"));
    gtk_box_append(inner, GTK_WIDGET(r2));

    /* Row 3: Architecture + System */
    GtkBox *r3 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_box_append(r3, stat_card("🏗", uts.machine, "Architecture"));
    gtk_box_append(r3, stat_card("🔤", uts.sysname, "System Name"));
    gtk_box_append(inner, GTK_WIDGET(r3));

    /* Row 4: RAM and Disk usage bars */
    GtkBox *r4 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));

    double ram_frac = mem_total > 0 ? (double)mem_used / (double)mem_total : 0.0;
    char ram_detail[64];
    snprintf(ram_detail, sizeof(ram_detail), "%lu MB used / %lu MB total", mem_used, mem_total);
    gtk_box_append(r4, usage_bar("MEMORY", ram_detail, ram_frac, "ram"));

    double disk_frac = disk_total_gb > 0 ? (double)disk_used_gb / (double)disk_total_gb : 0.0;
    char disk_detail[64];
    snprintf(disk_detail, sizeof(disk_detail), "%lu GB used / %lu GB total", disk_used_gb, disk_total_gb);
    gtk_box_append(r4, usage_bar("ROOT DISK", disk_detail, disk_frac, "disk"));

    gtk_box_append(inner, GTK_WIDGET(r4));

    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(inner, GTK_WIDGET(spacer));

    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(inner));
    gtk_box_append(page->box, GTK_WIDGET(scroll));

    free(os_pretty);
    free(cpu_model);
    return page;
}
