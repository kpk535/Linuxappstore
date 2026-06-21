#include "sysinfo.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

/* ── system data readers ───────────────────────────────────── */

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

static int get_cpu_cores(void) {
    int cores = 0;
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f) return 1;
    char line[256];
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, "processor", 9) == 0) cores++;
    fclose(f);
    return cores > 0 ? cores : 1;
}

static void get_memory_mb(unsigned long *total, unsigned long *used, unsigned long *avail) {
    *total = 0; *used = 0; *avail = 0;
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    char line[256];
    unsigned long kb;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %lu kB",     &kb) == 1) *total = kb / 1024;
        if (sscanf(line, "MemAvailable: %lu kB", &kb) == 1) *avail = kb / 1024;
    }
    fclose(f);
    if (*total > *avail) *used = *total - *avail;
}

static char *get_uptime_str(void) {
    double secs = 0;
    FILE *f = fopen("/proc/uptime", "r");
    if (f) { (void)fscanf(f, "%lf", &secs); fclose(f); }
    long days  = (long)secs / 86400;
    long hours = ((long)secs % 86400) / 3600;
    long mins  = ((long)secs % 3600)  / 60;
    char buf[64];
    if (days > 0)       snprintf(buf, sizeof(buf), "%ld d %ld h %ld m", days, hours, mins);
    else if (hours > 0) snprintf(buf, sizeof(buf), "%ld h %ld m", hours, mins);
    else                snprintf(buf, sizeof(buf), "%ld min", mins);
    return strdup(buf);
}

/* ── widget helpers ────────────────────────────────────────── */

static GtkWidget *stat_card(const char *icon, const char *value, const char *label,
                             GtkLabel **out_val) {
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

    if (out_val) *out_val = val;
    return GTK_WIDGET(card);
}

static GtkWidget *usage_bar(const char *label, const char *detail,
                             double fraction, const char *bar_css,
                             GtkProgressBar **out_bar,
                             GtkLabel **out_pct,
                             GtkLabel **out_det) {
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

    if (out_bar) *out_bar = bar;
    if (out_pct) *out_pct = pct;
    if (out_det) *out_det = det;
    return GTK_WIDGET(box);
}

/* ── refresh (update dynamic values) ──────────────────────── */

void page_sysinfo_refresh(PageSysinfo *page) {
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

    /* RAM */
    double ram_frac = mem_total > 0 ? (double)mem_used / (double)mem_total : 0.0;
    char ram_det[80];
    snprintf(ram_det, sizeof(ram_det), "%lu MB used / %lu MB total", mem_used, mem_total);
    gtk_label_set_text(page->ram_det, ram_det);
    gtk_progress_bar_set_fraction(page->ram_bar, ram_frac);
    char ram_pct[16];
    snprintf(ram_pct, sizeof(ram_pct), "%.0f%%", ram_frac * 100.0);
    gtk_label_set_text(page->ram_pct, ram_pct);

    /* Disk */
    double disk_frac = disk_total_gb > 0 ? (double)disk_used_gb / (double)disk_total_gb : 0.0;
    char disk_det[80];
    snprintf(disk_det, sizeof(disk_det), "%lu GB used / %lu GB total", disk_used_gb, disk_total_gb);
    gtk_label_set_text(page->disk_det, disk_det);
    gtk_progress_bar_set_fraction(page->disk_bar, disk_frac);
    char disk_pct[16];
    snprintf(disk_pct, sizeof(disk_pct), "%.0f%%", disk_frac * 100.0);
    gtk_label_set_text(page->disk_pct, disk_pct);

    /* Uptime */
    char *ut = get_uptime_str();
    gtk_label_set_text(page->uptime_val, ut);
    free(ut);
}

static void on_sysinfo_refresh(GtkButton *btn, gpointer data) {
    (void)btn;
    page_sysinfo_refresh((PageSysinfo *)data);
}

/* ── constructor ───────────────────────────────────────────── */

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

    /* ── Title + refresh button ── */
    GtkBox *title_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<span size='x-large'><b>System Info</b></span>");
    gtk_label_set_xalign(title, 0.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(title), TRUE);
    gtk_box_append(title_row, GTK_WIDGET(title));

    GtkButton *refresh_btn = GTK_BUTTON(gtk_button_new_with_label("⟳  Refresh"));
    gtk_box_append(title_row, GTK_WIDGET(refresh_btn));
    gtk_box_append(inner, GTK_WIDGET(title_row));

    GtkLabel *sub = GTK_LABEL(gtk_label_new(
        "Hardware and operating system details for this machine."));
    gtk_widget_add_css_class(GTK_WIDGET(sub), "page-subtitle");
    gtk_label_set_xalign(sub, 0.0f);
    gtk_box_append(inner, GTK_WIDGET(sub));

    /* ── Collect static data ── */
    struct utsname uts;
    uname(&uts);

    char hostname[256] = "unknown";
    gethostname(hostname, sizeof(hostname));

    char *os_pretty = read_os_field("PRETTY_NAME");
    char *cpu_model = get_cpu_model();
    int   cpu_cores = get_cpu_cores();

    /* ── Row 1: Hostname + OS ── */
    GtkBox *r1 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_box_append(r1, stat_card("🖥",  hostname,              "Hostname",         NULL));
    gtk_box_append(r1, stat_card("🐧",  os_pretty ?: "Linux",  "Operating System", NULL));
    gtk_box_append(inner, GTK_WIDGET(r1));

    /* ── Row 2: Kernel + Architecture ── */
    GtkBox *r2 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_box_append(r2, stat_card("⚙",  uts.release,  "Kernel",       NULL));
    gtk_box_append(r2, stat_card("🏗",  uts.machine,  "Architecture", NULL));
    gtk_box_append(inner, GTK_WIDGET(r2));

    /* ── Row 3: CPU model + core count ── */
    GtkBox *r3 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    char cpu_short[128];
    snprintf(cpu_short, sizeof(cpu_short), "%.126s", cpu_model);
    gtk_box_append(r3, stat_card("🔧", cpu_short, "Processor", NULL));
    char core_str[16];
    snprintf(core_str, sizeof(core_str), "%d", cpu_cores);
    gtk_box_append(r3, stat_card("⚡", core_str, "CPU Cores", NULL));
    gtk_box_append(inner, GTK_WIDGET(r3));

    /* ── Row 4: Uptime + System Name ── */
    GtkBox *r4 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));

    char *uptime_str = get_uptime_str();
    GtkWidget *uptime_card = stat_card("⏱", uptime_str, "Uptime", &page->uptime_val);
    free(uptime_str);
    gtk_box_append(r4, uptime_card);
    gtk_box_append(r4, stat_card("🔤", uts.sysname, "System Name", NULL));
    gtk_box_append(inner, GTK_WIDGET(r4));

    /* ── Row 5: RAM usage bar + Disk usage bar ── */
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

    GtkBox *r5 = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));

    double ram_frac = mem_total > 0 ? (double)mem_used / (double)mem_total : 0.0;
    char ram_det[80];
    snprintf(ram_det, sizeof(ram_det), "%lu MB used / %lu MB total", mem_used, mem_total);
    gtk_box_append(r5, usage_bar("MEMORY", ram_det, ram_frac, "ram",
                                 &page->ram_bar, &page->ram_pct, &page->ram_det));

    double disk_frac = disk_total_gb > 0 ? (double)disk_used_gb / (double)disk_total_gb : 0.0;
    char disk_det[80];
    snprintf(disk_det, sizeof(disk_det), "%lu GB used / %lu GB total", disk_used_gb, disk_total_gb);
    gtk_box_append(r5, usage_bar("ROOT DISK", disk_det, disk_frac, "disk",
                                 &page->disk_bar, &page->disk_pct, &page->disk_det));

    gtk_box_append(inner, GTK_WIDGET(r5));

    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(inner, GTK_WIDGET(spacer));

    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(inner));
    gtk_box_append(page->box, GTK_WIDGET(scroll));

    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_sysinfo_refresh), page);

    free(os_pretty);
    free(cpu_model);
    return page;
}
