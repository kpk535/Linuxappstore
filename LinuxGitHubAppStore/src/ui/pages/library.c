#include "library.h"
#include "../../services/package_db.h"
#include "../../services/installer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
        if (strcmp(d->install_type, "appimage") == 0) type = INSTALL_TYPE_APPIMAGE;
        else if (strcmp(d->install_type, "deb") == 0) type = INSTALL_TYPE_DEB;
        else if (strcmp(d->install_type, "tarball") == 0 ||
                 strcmp(d->install_type, "tar") == 0 ||
                 strcmp(d->install_type, "zip") == 0) type = INSTALL_TYPE_TARBALL;
        else if (strcmp(d->install_type, "rpm") == 0) type = INSTALL_TYPE_RPM;
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
    system(cmd);
}

void page_library_refresh(PageLibrary *page) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->apps_list))))
        gtk_list_box_remove(page->apps_list, child);

    int count = 0;
    InstalledApp **apps = package_db_load(&count);

    if (!apps || count == 0) {
        gtk_label_set_text(page->hint,
            "No installed apps yet. Find apps on the Search page and click Install.");
        free(apps);
        return;
    }

    char summary[64];
    snprintf(summary, sizeof(summary), "%d app%s installed", count, count == 1 ? "" : "s");
    gtk_label_set_text(page->hint, summary);

    for (int i = 0; i < count; i++) {
        InstalledApp *app = apps[i];

        GtkBox *row_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
        gtk_widget_set_margin_start(GTK_WIDGET(row_box), 16);
        gtk_widget_set_margin_end(GTK_WIDGET(row_box), 16);
        gtk_widget_set_margin_top(GTK_WIDGET(row_box), 12);
        gtk_widget_set_margin_bottom(GTK_WIDGET(row_box), 12);

        /* App icon placeholder */
        GtkLabel *icon = GTK_LABEL(gtk_label_new("📦"));
        gtk_widget_set_valign(GTK_WIDGET(icon), GTK_ALIGN_CENTER);
        gtk_box_append(row_box, GTK_WIDGET(icon));

        /* Info column */
        GtkBox *info = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        gtk_widget_set_hexpand(GTK_WIDGET(info), TRUE);

        /* Name + version */
        GtkBox *name_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
        char name_markup[256];
        snprintf(name_markup, sizeof(name_markup),
                 "<b>%s</b>", app->name ?: app->full_name ?: "Unknown");
        GtkLabel *name_lbl = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(name_lbl, name_markup);
        gtk_label_set_xalign(name_lbl, 0.0f);
        gtk_widget_set_hexpand(GTK_WIDGET(name_lbl), TRUE);
        gtk_box_append(name_row, GTK_WIDGET(name_lbl));

        /* Type badge */
        if (app->install_type) {
            const char *badge_class = "badge-lang";
            const char *badge_text  = app->install_type;
            if (strcmp(app->install_type, "appimage") == 0) badge_class = "badge-appimage";
            else if (strcmp(app->install_type, "deb") == 0) badge_class = "badge-deb";
            else if (strcmp(app->install_type, "rpm") == 0) badge_class = "badge-rpm";
            else if (strcmp(app->install_type, "tarball") == 0 ||
                     strcmp(app->install_type, "tar") == 0) badge_class = "badge-tar";
            else if (strcmp(app->install_type, "zip") == 0) badge_class = "badge-zip";
            GtkLabel *badge = GTK_LABEL(gtk_label_new(badge_text));
            gtk_widget_add_css_class(GTK_WIDGET(badge), badge_class);
            gtk_widget_set_valign(GTK_WIDGET(badge), GTK_ALIGN_CENTER);
            gtk_box_append(name_row, GTK_WIDGET(badge));
        }
        gtk_box_append(info, GTK_WIDGET(name_row));

        /* Version + full_name line */
        GtkBox *meta_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
        if (app->version) {
            char ver[64];
            snprintf(ver, sizeof(ver), "Version: %s", app->version);
            GtkLabel *ver_lbl = GTK_LABEL(gtk_label_new(ver));
            gtk_widget_add_css_class(GTK_WIDGET(ver_lbl), "muted");
            gtk_label_set_xalign(ver_lbl, 0.0f);
            gtk_box_append(meta_row, GTK_WIDGET(ver_lbl));
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
            GtkLabel *path_lbl = GTK_LABEL(gtk_label_new(app->install_path));
            gtk_label_set_xalign(path_lbl, 0.0f);
            gtk_label_set_ellipsize(path_lbl, PANGO_ELLIPSIZE_MIDDLE);
            gtk_widget_add_css_class(GTK_WIDGET(path_lbl), "muted");
            gtk_box_append(info, GTK_WIDGET(path_lbl));
        }

        /* Update badge */
        if (app->has_update && app->latest_version) {
            char upd[128];
            snprintf(upd, sizeof(upd), "⬆ Update available: %s", app->latest_version);
            GtkLabel *upd_lbl = GTK_LABEL(gtk_label_new(NULL));
            char upd_markup[160];
            snprintf(upd_markup, sizeof(upd_markup),
                     "<span>⬆ Update available: <b>%s</b></span>", app->latest_version);
            gtk_label_set_markup(upd_lbl, upd_markup);
            gtk_widget_add_css_class(GTK_WIDGET(upd_lbl), "badge-update");
            gtk_label_set_xalign(upd_lbl, 0.0f);
            gtk_widget_set_halign(GTK_WIDGET(upd_lbl), GTK_ALIGN_START);
            gtk_widget_set_margin_top(GTK_WIDGET(upd_lbl), 2);
            gtk_box_append(info, GTK_WIDGET(upd_lbl));
            (void)upd;
        }

        gtk_box_append(row_box, GTK_WIDGET(info));

        /* Action buttons */
        GtkBox *btns = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
        gtk_widget_set_valign(GTK_WIDGET(btns), GTK_ALIGN_CENTER);

        /* Launch button for AppImages */
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

PageLibrary *page_library_new(void) {
    PageLibrary *page = calloc(1, sizeof(PageLibrary));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 16));
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

    GtkButton *refresh_btn = GTK_BUTTON(gtk_button_new_with_label("⟳  Refresh"));
    gtk_box_append(hdr, GTK_WIDGET(refresh_btn));
    gtk_box_append(page->box, GTK_WIDGET(hdr));

    page->hint = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->hint, 0.0f);
    gtk_label_set_wrap(page->hint, TRUE);
    gtk_widget_add_css_class(GTK_WIDGET(page->hint), "page-subtitle");
    gtk_box_append(page->box, GTK_WIDGET(page->hint));

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
