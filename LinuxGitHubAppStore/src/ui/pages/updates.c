#include "updates.h"
#include "../../services/package_db.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *acolors[] = {
    "avatar-blue","avatar-green","avatar-purple",
    "avatar-red","avatar-orange","avatar-pink","avatar-teal"
};

static void rebuild_list(PageUpdates *page, InstalledApp **apps, int count) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->list))))
        gtk_list_box_remove(page->list, child);

    /* Empty state: no installed apps */
    if (count == 0) {
        GtkBox *empty = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
        gtk_widget_set_halign(GTK_WIDGET(empty), GTK_ALIGN_CENTER);
        gtk_widget_set_valign(GTK_WIDGET(empty), GTK_ALIGN_CENTER);
        gtk_widget_set_vexpand(GTK_WIDGET(empty), TRUE);
        gtk_widget_set_margin_top(GTK_WIDGET(empty), 48);

        GtkLabel *ico = GTK_LABEL(gtk_label_new("📦"));
        gtk_widget_add_css_class(GTK_WIDGET(ico), "empty-icon");
        gtk_label_set_xalign(ico, 0.5f);
        gtk_box_append(empty, GTK_WIDGET(ico));

        GtkLabel *msg = GTK_LABEL(gtk_label_new("No installed apps"));
        gtk_widget_add_css_class(GTK_WIDGET(msg), "empty-text");
        gtk_label_set_xalign(msg, 0.5f);
        gtk_box_append(empty, GTK_WIDGET(msg));

        GtkLabel *hint = GTK_LABEL(gtk_label_new("Install apps from the Search page first."));
        gtk_widget_add_css_class(GTK_WIDGET(hint), "empty-hint");
        gtk_label_set_xalign(hint, 0.5f);
        gtk_box_append(empty, GTK_WIDGET(hint));

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_selectable(row, FALSE);
        gtk_list_box_row_set_child(row, GTK_WIDGET(empty));
        gtk_list_box_append(page->list, GTK_WIDGET(row));

        gtk_label_set_text(page->hint, "No apps installed yet.");
        gtk_widget_remove_css_class(GTK_WIDGET(page->hint), "warning");
        return;
    }

    int updates = 0;
    for (int i = 0; i < count; i++)
        if (apps[i]->has_update) updates++;

    gtk_widget_remove_css_class(GTK_WIDGET(page->hint), "warning");
    if (updates == 0) {
        gtk_label_set_text(page->hint, "All apps are up to date.");
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "%d update%s available.",
                 updates, updates == 1 ? "" : "s");
        gtk_label_set_text(page->hint, msg);
        gtk_widget_add_css_class(GTK_WIDGET(page->hint), "warning");
    }

    for (int i = 0; i < count; i++) {
        InstalledApp *app = apps[i];
        const char *display = app->name ?: app->full_name ?: "?";

        GtkBox *row_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
        gtk_widget_set_margin_start(GTK_WIDGET(row_box), 14);
        gtk_widget_set_margin_end(GTK_WIDGET(row_box), 14);
        gtk_widget_set_margin_top(GTK_WIDGET(row_box), 10);
        gtk_widget_set_margin_bottom(GTK_WIDGET(row_box), 10);

        /* Avatar circle */
        char init_ch[4];
        init_ch[0] = (display[0]) ? (char)g_ascii_toupper((guchar)display[0]) : '?';
        init_ch[1] = '\0';
        GtkLabel *avatar = GTK_LABEL(gtk_label_new(init_ch));
        gtk_widget_add_css_class(GTK_WIDGET(avatar), "app-avatar");
        gtk_widget_add_css_class(GTK_WIDGET(avatar),
            acolors[(unsigned char)init_ch[0] % 7]);
        gtk_widget_set_valign(GTK_WIDGET(avatar), GTK_ALIGN_CENTER);
        gtk_box_append(row_box, GTK_WIDGET(avatar));

        /* Info column */
        GtkBox *info = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        gtk_widget_set_hexpand(GTK_WIDGET(info), TRUE);

        GtkLabel *name_lbl = GTK_LABEL(gtk_label_new(display));
        gtk_widget_add_css_class(GTK_WIDGET(name_lbl), "stat-value");
        gtk_label_set_xalign(name_lbl, 0.0f);
        gtk_label_set_ellipsize(name_lbl, PANGO_ELLIPSIZE_END);
        gtk_box_append(info, GTK_WIDGET(name_lbl));

        /* Version row */
        GtkBox *ver_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6));
        gtk_widget_set_valign(GTK_WIDGET(ver_row), GTK_ALIGN_CENTER);

        if (app->has_update && app->latest_version) {
            GtkLabel *old_ver = GTK_LABEL(gtk_label_new(app->version ?: "?"));
            gtk_widget_add_css_class(GTK_WIDGET(old_ver), "version-old");
            gtk_label_set_xalign(old_ver, 0.0f);
            gtk_box_append(ver_row, GTK_WIDGET(old_ver));

            GtkLabel *arrow = GTK_LABEL(gtk_label_new("→"));
            gtk_widget_add_css_class(GTK_WIDGET(arrow), "version-arrow");
            gtk_box_append(ver_row, GTK_WIDGET(arrow));

            GtkLabel *new_ver = GTK_LABEL(gtk_label_new(app->latest_version));
            gtk_widget_add_css_class(GTK_WIDGET(new_ver), "version-new");
            gtk_label_set_xalign(new_ver, 0.0f);
            gtk_box_append(ver_row, GTK_WIDGET(new_ver));
        } else {
            char ver_str[128];
            snprintf(ver_str, sizeof(ver_str), "v%s", app->version ?: "?");
            GtkLabel *vl = GTK_LABEL(gtk_label_new(ver_str));
            gtk_widget_add_css_class(GTK_WIDGET(vl), "muted");
            gtk_label_set_xalign(vl, 0.0f);
            gtk_box_append(ver_row, GTK_WIDGET(vl));
        }

        gtk_box_append(info, GTK_WIDGET(ver_row));
        gtk_box_append(row_box, GTK_WIDGET(info));

        /* Status badge */
        if (app->has_update) {
            GtkLabel *badge = GTK_LABEL(gtk_label_new("Update"));
            gtk_widget_add_css_class(GTK_WIDGET(badge), "badge-update");
            gtk_widget_set_valign(GTK_WIDGET(badge), GTK_ALIGN_CENTER);
            gtk_box_append(row_box, GTK_WIDGET(badge));
        } else if (app->latest_version) {
            GtkLabel *badge = GTK_LABEL(gtk_label_new("Up to date"));
            gtk_widget_add_css_class(GTK_WIDGET(badge), "badge-ok");
            gtk_widget_set_valign(GTK_WIDGET(badge), GTK_ALIGN_CENTER);
            gtk_box_append(row_box, GTK_WIDGET(badge));
        }

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_selectable(row, FALSE);
        gtk_list_box_row_set_child(row, GTK_WIDGET(row_box));
        gtk_list_box_append(page->list, GTK_WIDGET(row));
    }
}

static void on_check_updates(GtkButton *btn, gpointer data) {
    (void)btn;
    PageUpdates *page = (PageUpdates *)data;
    gtk_label_set_text(page->hint, "Checking for updates…");
    gtk_widget_remove_css_class(GTK_WIDGET(page->hint), "warning");
    gtk_widget_set_sensitive(GTK_WIDGET(page->check_btn), FALSE);
    gtk_spinner_start(page->spinner);
    gtk_widget_set_visible(GTK_WIDGET(page->spinner), TRUE);

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
            app->has_update     = has_update;
            release_free(latest);
        }
    }

    github_service_free(svc);
    gtk_spinner_stop(page->spinner);
    gtk_widget_set_visible(GTK_WIDGET(page->spinner), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(page->check_btn), TRUE);

    rebuild_list(page, apps, count);
    for (int i = 0; i < count; i++) installed_app_free(apps[i]);
    free(apps);
}

PageUpdates *page_updates_new(void) {
    PageUpdates *page = calloc(1, sizeof(PageUpdates));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 16));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_bottom(GTK_WIDGET(page->box), 16);

    /* Header */
    GtkBox *header = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));

    GtkLabel *title = GTK_LABEL(gtk_label_new("Updates"));
    gtk_widget_add_css_class(GTK_WIDGET(title), "page-title");
    gtk_label_set_xalign(title, 0.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(title), TRUE);
    gtk_box_append(header, GTK_WIDGET(title));

    page->spinner = GTK_SPINNER(gtk_spinner_new());
    gtk_widget_set_visible(GTK_WIDGET(page->spinner), FALSE);
    gtk_widget_set_valign(GTK_WIDGET(page->spinner), GTK_ALIGN_CENTER);
    gtk_box_append(header, GTK_WIDGET(page->spinner));

    page->check_btn = GTK_BUTTON(gtk_button_new_with_label("Check for Updates"));
    gtk_widget_add_css_class(GTK_WIDGET(page->check_btn), "suggested-action");
    gtk_box_append(header, GTK_WIDGET(page->check_btn));
    gtk_box_append(page->box, GTK_WIDGET(header));

    page->hint = GTK_LABEL(gtk_label_new(
        "Click 'Check for Updates' to scan all installed apps for new releases."));
    gtk_label_set_xalign(page->hint, 0.0f);
    gtk_label_set_wrap(page->hint, TRUE);
    gtk_widget_add_css_class(GTK_WIDGET(page->hint), "page-subtitle");
    gtk_box_append(page->box, GTK_WIDGET(page->hint));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    page->list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_activate_on_single_click(page->list, FALSE);
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(page->list));
    gtk_box_append(page->box, GTK_WIDGET(scroll));

    g_signal_connect(page->check_btn, "clicked", G_CALLBACK(on_check_updates), page);

    int count = 0;
    InstalledApp **apps = package_db_load(&count);
    rebuild_list(page, apps, count);
    for (int i = 0; i < count; i++) installed_app_free(apps[i]);
    free(apps);

    return page;
}
