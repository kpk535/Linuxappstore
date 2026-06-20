#include "library.h"
#include "../../services/package_db.h"
#include "../../services/installer.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    PageLibrary *page;
    char        *full_name;
    char        *install_path;
    char        *install_type;
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
        else if (strcmp(d->install_type, "deb") == 0)  type = INSTALL_TYPE_DEB;
        else if (strcmp(d->install_type, "tarball") == 0 ||
                 strcmp(d->install_type, "zip") == 0)  type = INSTALL_TYPE_TARBALL;
    }

    installer_uninstall(d->install_path, type);
    package_db_remove(d->full_name);

    /* Remove the row from the list */
    gtk_list_box_remove(GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(d->row))),
                        GTK_WIDGET(d->row));

    page_library_refresh(d->page);
}

void page_library_refresh(PageLibrary *page) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->apps_list))))
        gtk_list_box_remove(page->apps_list, child);

    int count = 0;
    InstalledApp **apps = package_db_load(&count);

    if (!apps || count == 0) {
        gtk_label_set_text(page->hint, "No installed apps yet. Find and install apps from the Search page.");
        free(apps);
        return;
    }

    gtk_label_set_text(page->hint, "");

    for (int i = 0; i < count; i++) {
        InstalledApp *app = apps[i];

        GtkBox *row_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));
        gtk_widget_set_margin_start(GTK_WIDGET(row_box), 12);
        gtk_widget_set_margin_end(GTK_WIDGET(row_box), 12);
        gtk_widget_set_margin_top(GTK_WIDGET(row_box), 10);
        gtk_widget_set_margin_bottom(GTK_WIDGET(row_box), 10);

        GtkBox *info = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
        gtk_widget_set_hexpand(GTK_WIDGET(info), TRUE);

        char title_markup[256];
        snprintf(title_markup, sizeof(title_markup),
                 "<b>%s</b>   <small>%s</small>",
                 app->name ?: app->full_name ?: "?",
                 app->version ?: "unknown");
        GtkLabel *name_lbl = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(name_lbl, title_markup);
        gtk_label_set_xalign(name_lbl, 0.0f);
        gtk_box_append(info, GTK_WIDGET(name_lbl));

        if (app->install_path) {
            GtkLabel *path_lbl = GTK_LABEL(gtk_label_new(app->install_path));
            gtk_label_set_xalign(path_lbl, 0.0f);
            gtk_label_set_ellipsize(path_lbl, PANGO_ELLIPSIZE_MIDDLE);
            gtk_box_append(info, GTK_WIDGET(path_lbl));
        }

        if (app->has_update && app->latest_version) {
            char upd[128];
            snprintf(upd, sizeof(upd), "⬆ Update available: %s", app->latest_version);
            GtkLabel *upd_lbl = GTK_LABEL(gtk_label_new(upd));
            gtk_label_set_xalign(upd_lbl, 0.0f);
            gtk_box_append(info, GTK_WIDGET(upd_lbl));
        }

        gtk_box_append(row_box, GTK_WIDGET(info));

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

        gtk_box_append(row_box, GTK_WIDGET(uninstall_btn));
        gtk_list_box_append(page->apps_list, GTK_WIDGET(row));

        installed_app_free(app);
    }

    free(apps);
}

PageLibrary *page_library_new(void) {
    PageLibrary *page = calloc(1, sizeof(PageLibrary));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 20);

    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<span size='x-large'><b>Library</b></span>");
    gtk_label_set_xalign(title, 0.0f);
    gtk_box_append(page->box, GTK_WIDGET(title));

    page->hint = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->hint, 0.0f);
    gtk_label_set_wrap(page->hint, TRUE);
    gtk_box_append(page->box, GTK_WIDGET(page->hint));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    page->apps_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(page->apps_list));
    gtk_box_append(page->box, GTK_WIDGET(scroll));

    page_library_refresh(page);
    return page;
}
