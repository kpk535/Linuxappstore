#include "library.h"
#include "../../services/package_db.h"
#include "../../services/installer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── filter helpers ────────────────────────────────────────── */

static const char *FILTER_KEYS[LIBRARY_N_FILTERS] = {
    "all", "appimage", "deb", "rpm", "tar"
};
static const char *FILTER_LABELS[LIBRARY_N_FILTERS] = {
    "All", "AppImage", ".deb", ".rpm", ".tar / .zip"
};

static int app_matches_filter(InstalledApp *app, const char *filter) {
    if (!filter || strcmp(filter, "all") == 0) return 1;
    if (!app->install_type) return 0;
    if (strcmp(filter, "appimage") == 0)
        return strcmp(app->install_type, "appimage") == 0;
    if (strcmp(filter, "deb") == 0)
        return strcmp(app->install_type, "deb") == 0;
    if (strcmp(filter, "rpm") == 0)
        return strcmp(app->install_type, "rpm") == 0;
    if (strcmp(filter, "tar") == 0)
        return strcmp(app->install_type, "tarball") == 0 ||
               strcmp(app->install_type, "tar")     == 0 ||
               strcmp(app->install_type, "zip")     == 0;
    return 1;
}

/* ── uninstall ─────────────────────────────────────────────── */

typedef struct {
    PageLibrary   *page;
    char          *full_name;
    char          *install_path;
    char          *install_type;
    GtkListBoxRow *row;
} UninstallData;

static void uninstall_data_free(gpointer data) {
    UninstallData *d = (UninstallData *)data;
    free(d->full_name);
    free(d->install_path);
    free(d->install_type);
    free(d);
}

static void on_uninstall(GtkButton *btn, gpointer data) {
    (void)btn;
    UninstallData *d = (UninstallData *)data;

    InstallType type = INSTALL_TYPE_UNKNOWN;
    if (d->install_type) {
        if      (strcmp(d->install_type, "appimage") == 0) type = INSTALL_TYPE_APPIMAGE;
        else if (strcmp(d->install_type, "deb") == 0)      type = INSTALL_TYPE_DEB;
        else if (strcmp(d->install_type, "rpm") == 0)      type = INSTALL_TYPE_RPM;
        else if (strcmp(d->install_type, "tarball") == 0 ||
                 strcmp(d->install_type, "tar") == 0 ||
                 strcmp(d->install_type, "zip") == 0)      type = INSTALL_TYPE_TARBALL;
    }

    installer_uninstall(d->install_path, type);
    package_db_remove(d->full_name);
    gtk_list_box_remove(GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(d->row))),
                        GTK_WIDGET(d->row));
    page_library_refresh(d->page);
}

static void on_launch(GtkButton *btn, gpointer data) {
    (void)btn;
    const char *path = (const char *)data;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "\"%s\" &", path);
    (void)system(cmd);
}

/* ── filter tab callback ───────────────────────────────────── */

typedef struct {
    PageLibrary *page;
    int          idx;
} FilterTabData;

static void filter_tab_data_free(gpointer data) { free(data); }

static void on_filter_tab(GtkButton *btn, gpointer data) {
    (void)btn;
    FilterTabData *d = (FilterTabData *)data;
    PageLibrary *p = d->page;
    p->active_filter = FILTER_KEYS[d->idx];

    for (int i = 0; i < LIBRARY_N_FILTERS; i++)
        gtk_widget_remove_css_class(GTK_WIDGET(p->filter_btns[i]), "filter-tab-active");
    gtk_widget_add_css_class(GTK_WIDGET(p->filter_btns[d->idx]), "filter-tab-active");

    page_library_refresh(p);
}

/* ── refresh ───────────────────────────────────────────────── */

void page_library_refresh(PageLibrary *page) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->apps_list))))
        gtk_list_box_remove(page->apps_list, child);

    int total = 0;
    InstalledApp **apps = package_db_load(&total);
    const char *filter  = page->active_filter ?: "all";

    int visible = 0;
    if (apps)
        for (int i = 0; i < total; i++)
            if (app_matches_filter(apps[i], filter)) visible++;

    /* Count label */
    if (page->count_label) {
        char cnt[32];
        if (total == 0)
            snprintf(cnt, sizeof(cnt), "0 apps");
        else if (strcmp(filter, "all") == 0)
            snprintf(cnt, sizeof(cnt), "%d", total);
        else
            snprintf(cnt, sizeof(cnt), "%d / %d", visible, total);
        gtk_label_set_text(page->count_label, cnt);
    }

    /* Empty state */
    if (!apps || total == 0) {
        GtkBox *empty = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 14));
        gtk_widget_set_halign(GTK_WIDGET(empty), GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(GTK_WIDGET(empty), 60);

        GtkLabel *ico = GTK_LABEL(gtk_label_new("📦"));
        gtk_widget_add_css_class(GTK_WIDGET(ico), "empty-icon");
        gtk_label_set_xalign(ico, 0.5f);
        gtk_box_append(empty, GTK_WIDGET(ico));

        GtkLabel *et = GTK_LABEL(gtk_label_new("No apps installed yet"));
        gtk_widget_add_css_class(GTK_WIDGET(et), "empty-text");
        gtk_label_set_xalign(et, 0.5f);
        gtk_box_append(empty, GTK_WIDGET(et));

        GtkLabel *eh = GTK_LABEL(gtk_label_new("Use Search to find apps and click Install"));
        gtk_widget_add_css_class(GTK_WIDGET(eh), "empty-hint");
        gtk_label_set_xalign(eh, 0.5f);
        gtk_box_append(empty, GTK_WIDGET(eh));

        GtkListBoxRow *erow = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_selectable(erow, FALSE);
        gtk_list_box_row_set_child(erow, GTK_WIDGET(empty));
        gtk_list_box_append(page->apps_list, GTK_WIDGET(erow));

        gtk_label_set_text(page->hint, "");
        free(apps);
        return;
    }

    /* Hint */
    if (visible == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "No %s apps installed.", filter);
        gtk_label_set_text(page->hint, msg);
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "%d app%s installed", visible, visible == 1 ? "" : "s");
        gtk_label_set_text(page->hint, msg);
    }

    static const char *acolors[] = {
        "avatar-blue","avatar-green","avatar-purple",
        "avatar-red","avatar-orange","avatar-pink","avatar-teal"
    };

    for (int i = 0; i < total; i++) {
        InstalledApp *app = apps[i];

        if (!app_matches_filter(app, filter)) {
            installed_app_free(app);
            continue;
        }

        GtkBox *row_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
        gtk_widget_set_margin_start(GTK_WIDGET(row_box), 16);
        gtk_widget_set_margin_end(GTK_WIDGET(row_box), 16);
        gtk_widget_set_margin_top(GTK_WIDGET(row_box), 12);
        gtk_widget_set_margin_bottom(GTK_WIDGET(row_box), 12);

        /* Avatar */
        const char *an = app->name ?: app->full_name ?: "?";
        char init_ch[4];
        init_ch[0] = g_ascii_toupper((guchar)an[0]); init_ch[1] = '\0';
        GtkLabel *avatar = GTK_LABEL(gtk_label_new(init_ch));
        gtk_widget_add_css_class(GTK_WIDGET(avatar), "app-avatar");
        gtk_widget_add_css_class(GTK_WIDGET(avatar), acolors[g_ascii_toupper((guchar)init_ch[0]) % 7]);
        gtk_widget_set_valign(GTK_WIDGET(avatar), GTK_ALIGN_CENTER);
        gtk_widget_set_halign(GTK_WIDGET(avatar), GTK_ALIGN_CENTER);
        gtk_box_append(row_box, GTK_WIDGET(avatar));

        /* Info column */
        GtkBox *info = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        gtk_widget_set_hexpand(GTK_WIDGET(info), TRUE);

        /* Name + type badge */
        GtkBox *name_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
        char name_markup[256];
        snprintf(name_markup, sizeof(name_markup),
                 "<b>%s</b>", app->name ?: app->full_name ?: "Unknown");
        GtkLabel *name_lbl = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(name_lbl, name_markup);
        gtk_label_set_xalign(name_lbl, 0.0f);
        gtk_widget_set_hexpand(GTK_WIDGET(name_lbl), TRUE);
        gtk_box_append(name_row, GTK_WIDGET(name_lbl));

        if (app->install_type) {
            const char *bc = "badge-lang";
            if      (strcmp(app->install_type, "appimage") == 0) bc = "badge-appimage";
            else if (strcmp(app->install_type, "deb")      == 0) bc = "badge-deb";
            else if (strcmp(app->install_type, "rpm")      == 0) bc = "badge-rpm";
            else if (strcmp(app->install_type, "tarball")  == 0 ||
                     strcmp(app->install_type, "tar")      == 0) bc = "badge-tar";
            else if (strcmp(app->install_type, "zip")      == 0) bc = "badge-zip";
            GtkLabel *badge = GTK_LABEL(gtk_label_new(app->install_type));
            gtk_widget_add_css_class(GTK_WIDGET(badge), bc);
            gtk_widget_set_valign(GTK_WIDGET(badge), GTK_ALIGN_CENTER);
            gtk_box_append(name_row, GTK_WIDGET(badge));
        }
        gtk_box_append(info, GTK_WIDGET(name_row));

        /* Version + repo */
        GtkBox *meta_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
        if (app->version) {
            char ver[64];
            snprintf(ver, sizeof(ver), "v%s", app->version);
            GtkLabel *vl = GTK_LABEL(gtk_label_new(ver));
            gtk_widget_add_css_class(GTK_WIDGET(vl), "muted");
            gtk_label_set_xalign(vl, 0.0f);
            gtk_box_append(meta_row, GTK_WIDGET(vl));
        }
        if (app->full_name) {
            GtkLabel *fn = GTK_LABEL(gtk_label_new(app->full_name));
            gtk_widget_add_css_class(GTK_WIDGET(fn), "muted");
            gtk_label_set_xalign(fn, 0.0f);
            gtk_box_append(meta_row, GTK_WIDGET(fn));
        }
        gtk_box_append(info, GTK_WIDGET(meta_row));

        /* Install path */
        if (app->install_path) {
            GtkLabel *pl = GTK_LABEL(gtk_label_new(app->install_path));
            gtk_label_set_xalign(pl, 0.0f);
            gtk_label_set_ellipsize(pl, PANGO_ELLIPSIZE_MIDDLE);
            gtk_widget_add_css_class(GTK_WIDGET(pl), "info-mono");
            gtk_box_append(info, GTK_WIDGET(pl));
        }

        /* Update available badge */
        if (app->has_update && app->latest_version) {
            char upd_markup[160];
            snprintf(upd_markup, sizeof(upd_markup),
                     "<span>⬆ Update: <b>%s</b></span>", app->latest_version);
            GtkLabel *upd = GTK_LABEL(gtk_label_new(NULL));
            gtk_label_set_markup(upd, upd_markup);
            gtk_widget_add_css_class(GTK_WIDGET(upd), "badge-update");
            gtk_label_set_xalign(upd, 0.0f);
            gtk_widget_set_halign(GTK_WIDGET(upd), GTK_ALIGN_START);
            gtk_widget_set_margin_top(GTK_WIDGET(upd), 2);
            gtk_box_append(info, GTK_WIDGET(upd));
        }

        gtk_box_append(row_box, GTK_WIDGET(info));

        /* Action buttons */
        GtkBox *btns = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
        gtk_widget_set_valign(GTK_WIDGET(btns), GTK_ALIGN_CENTER);

        if (app->install_type && strcmp(app->install_type, "appimage") == 0
            && app->install_path) {
            GtkButton *launch_btn = GTK_BUTTON(gtk_button_new_with_label("▶  Launch"));
            gtk_widget_add_css_class(GTK_WIDGET(launch_btn), "suggested-action");
            char *path_copy = strdup(app->install_path);
            g_signal_connect_data(launch_btn, "clicked",
                                  G_CALLBACK(on_launch), path_copy,
                                  (GClosureNotify)free, 0);
            gtk_box_append(btns, GTK_WIDGET(launch_btn));
        }

        GtkButton *uninstall_btn = GTK_BUTTON(gtk_button_new_with_label("Uninstall"));
        gtk_widget_add_css_class(GTK_WIDGET(uninstall_btn), "destructive-action");

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_selectable(row, FALSE);
        gtk_list_box_row_set_child(row, GTK_WIDGET(row_box));

        UninstallData *ud = malloc(sizeof(UninstallData));
        ud->page         = page;
        ud->full_name    = app->full_name    ? strdup(app->full_name)    : NULL;
        ud->install_path = app->install_path ? strdup(app->install_path) : NULL;
        ud->install_type = app->install_type ? strdup(app->install_type) : NULL;
        ud->row          = row;

        g_signal_connect_data(uninstall_btn, "clicked",
                              G_CALLBACK(on_uninstall), ud,
                              (GClosureNotify)uninstall_data_free, 0);

        gtk_box_append(btns, GTK_WIDGET(uninstall_btn));
        gtk_box_append(row_box, GTK_WIDGET(btns));
        gtk_list_box_append(page->apps_list, GTK_WIDGET(row));

        installed_app_free(app);
    }
    free(apps);
}

/* ── constructor ───────────────────────────────────────────── */

PageLibrary *page_library_new(void) {
    PageLibrary *page = calloc(1, sizeof(PageLibrary));
    page->active_filter = "all";

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 12));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_bottom(GTK_WIDGET(page->box), 16);

    /* Header row */
    GtkBox *hdr = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));

    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<span size='x-large'><b>Library</b></span>");
    gtk_label_set_xalign(title, 0.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(title), TRUE);
    gtk_box_append(hdr, GTK_WIDGET(title));

    page->count_label = GTK_LABEL(gtk_label_new("0"));
    gtk_widget_add_css_class(GTK_WIDGET(page->count_label), "summary-count");
    gtk_widget_set_valign(GTK_WIDGET(page->count_label), GTK_ALIGN_CENTER);
    gtk_box_append(hdr, GTK_WIDGET(page->count_label));

    GtkButton *refresh_btn = GTK_BUTTON(gtk_button_new_with_label("⟳  Refresh"));
    gtk_box_append(hdr, GTK_WIDGET(refresh_btn));
    gtk_box_append(page->box, GTK_WIDGET(hdr));

    /* Subtitle hint */
    page->hint = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->hint, 0.0f);
    gtk_label_set_wrap(page->hint, TRUE);
    gtk_widget_add_css_class(GTK_WIDGET(page->hint), "page-subtitle");
    gtk_box_append(page->box, GTK_WIDGET(page->hint));

    /* Filter tab bar */
    GtkBox *filter_bar = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4));
    gtk_widget_add_css_class(GTK_WIDGET(filter_bar), "filter-bar");
    gtk_widget_set_halign(GTK_WIDGET(filter_bar), GTK_ALIGN_START);

    for (int i = 0; i < LIBRARY_N_FILTERS; i++) {
        GtkButton *tab = GTK_BUTTON(gtk_button_new_with_label(FILTER_LABELS[i]));
        gtk_widget_add_css_class(GTK_WIDGET(tab), "filter-tab");
        if (i == 0) gtk_widget_add_css_class(GTK_WIDGET(tab), "filter-tab-active");
        page->filter_btns[i] = tab;

        FilterTabData *ftd = malloc(sizeof(FilterTabData));
        ftd->page = page;
        ftd->idx  = i;
        g_signal_connect_data(tab, "clicked", G_CALLBACK(on_filter_tab), ftd,
                              (GClosureNotify)filter_tab_data_free, 0);
        gtk_box_append(filter_bar, GTK_WIDGET(tab));
    }
    gtk_box_append(page->box, GTK_WIDGET(filter_bar));

    /* Scrollable app list */
    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    page->apps_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_activate_on_single_click(page->apps_list, FALSE);
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(page->apps_list));
    gtk_box_append(page->box, GTK_WIDGET(scroll));

    g_signal_connect_swapped(refresh_btn, "clicked",
                             G_CALLBACK(page_library_refresh), page);

    page_library_refresh(page);
    return page;
}
