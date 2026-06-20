#include "updates.h"
#include "../../services/package_db.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include <stdlib.h>
#include <string.h>

static void rebuild_list(PageUpdates *page, InstalledApp **apps, int count) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->list))))
        gtk_list_box_remove(page->list, child);

    int updates = 0;
    for (int i = 0; i < count; i++) {
        InstalledApp *app = apps[i];

        GtkBox *row_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));
        gtk_widget_set_margin_start(GTK_WIDGET(row_box), 12);
        gtk_widget_set_margin_end(GTK_WIDGET(row_box), 12);
        gtk_widget_set_margin_top(GTK_WIDGET(row_box), 10);
        gtk_widget_set_margin_bottom(GTK_WIDGET(row_box), 10);

        GtkBox *info = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
        gtk_widget_set_hexpand(GTK_WIDGET(info), TRUE);

        char markup[256];
        snprintf(markup, sizeof(markup), "<b>%s</b>", app->name ?: app->full_name ?: "?");
        GtkLabel *name_lbl = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(name_lbl, markup);
        gtk_label_set_xalign(name_lbl, 0.0f);
        gtk_box_append(info, GTK_WIDGET(name_lbl));

        char ver_str[128];
        if (app->has_update && app->latest_version) {
            snprintf(ver_str, sizeof(ver_str),
                     "Installed: %s  →  Latest: %s",
                     app->version ?: "?", app->latest_version);
            updates++;
        } else {
            snprintf(ver_str, sizeof(ver_str),
                     "Installed: %s  ✓ Up to date", app->version ?: "?");
        }

        GtkLabel *ver_lbl = GTK_LABEL(gtk_label_new(ver_str));
        gtk_label_set_xalign(ver_lbl, 0.0f);
        gtk_box_append(info, GTK_WIDGET(ver_lbl));
        gtk_box_append(row_box, GTK_WIDGET(info));

        if (app->has_update) {
            GtkLabel *badge = GTK_LABEL(gtk_label_new("⬆ Update"));
            gtk_box_append(row_box, GTK_WIDGET(badge));
        }

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_selectable(row, FALSE);
        gtk_list_box_row_set_child(row, GTK_WIDGET(row_box));
        gtk_list_box_append(page->list, GTK_WIDGET(row));
    }

    if (count == 0) {
        gtk_label_set_text(page->hint, "No installed apps. Install apps from the Search page first.");
    } else if (updates == 0) {
        gtk_label_set_text(page->hint, "All apps are up to date.");
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "%d update%s available.", updates, updates == 1 ? "" : "s");
        gtk_label_set_text(page->hint, msg);
    }
}

static void on_check_updates(GtkButton *btn, gpointer data) {
    (void)btn;
    PageUpdates *page = (PageUpdates *)data;
    gtk_label_set_text(page->hint, "Checking for updates…");
    gtk_widget_set_sensitive(GTK_WIDGET(page->check_btn), FALSE);

    AppSettings *settings = settings_load();
    GitHubService *svc = github_service_new(settings->github_token);
    settings_free(settings);

    int count = 0;
    InstalledApp **apps = package_db_load(&count);

    for (int i = 0; i < count; i++) {
        InstalledApp *app = apps[i];
        if (!app->full_name) continue;

        GitHubRelease *latest = github_get_latest_release(svc, app->full_name);
        if (latest && latest->tag_name) {
            int has_update = app->version &&
                             strcmp(app->version, latest->tag_name) != 0;
            package_db_update_latest(app->full_name, latest->tag_name, has_update);
            free(app->latest_version);
            app->latest_version = strdup(latest->tag_name);
            app->has_update = has_update;
            release_free(latest);
        }
    }

    github_service_free(svc);

    rebuild_list(page, apps, count);
    for (int i = 0; i < count; i++) installed_app_free(apps[i]);
    free(apps);

    gtk_widget_set_sensitive(GTK_WIDGET(page->check_btn), TRUE);
}

PageUpdates *page_updates_new(void) {
    PageUpdates *page = calloc(1, sizeof(PageUpdates));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 20);

    GtkBox *header = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10));

    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<span size='x-large'><b>Updates</b></span>");
    gtk_label_set_xalign(title, 0.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(title), TRUE);
    gtk_box_append(header, GTK_WIDGET(title));

    page->check_btn = GTK_BUTTON(gtk_button_new_with_label("Check for Updates"));
    gtk_box_append(header, GTK_WIDGET(page->check_btn));
    gtk_box_append(page->box, GTK_WIDGET(header));

    page->hint = GTK_LABEL(gtk_label_new("Click 'Check for Updates' to check all installed apps."));
    gtk_label_set_xalign(page->hint, 0.0f);
    gtk_label_set_wrap(page->hint, TRUE);
    gtk_box_append(page->box, GTK_WIDGET(page->hint));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    page->list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_activate_on_single_click(page->list, FALSE);
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(page->list));
    gtk_box_append(page->box, GTK_WIDGET(scroll));

    g_signal_connect(page->check_btn, "clicked", G_CALLBACK(on_check_updates), page);

    /* Show current list on load */
    int count = 0;
    InstalledApp **apps = package_db_load(&count);
    rebuild_list(page, apps, count);
    for (int i = 0; i < count; i++) installed_app_free(apps[i]);
    free(apps);

    return page;
}
