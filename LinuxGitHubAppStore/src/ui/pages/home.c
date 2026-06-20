#include "home.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include "../../services/installer.h"
#include "../../services/package_db.h"
#include "../../models/release.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/* ── install progress dialog ──────────────────────────────── */

typedef struct {
    GtkWindow       *dialog;
    GtkProgressBar  *bar;
    GtkLabel        *status_label;
    GtkButton       *close_btn;
    InstallProgress *progress;
    guint            timer_id;
    InstallTask     *task;
} ProgressDialog;

static gboolean poll_progress(gpointer data) {
    ProgressDialog *pd = (ProgressDialog *)data;
    InstallProgress *p = pd->progress;

    gtk_progress_bar_set_fraction(pd->bar, p->progress);
    if (p->status) gtk_label_set_text(pd->status_label, p->status);

    if (p->done) {
        gtk_progress_bar_set_fraction(pd->bar, 1.0);
        gtk_widget_set_sensitive(GTK_WIDGET(pd->close_btn), TRUE);

        if (p->success && p->install_path) {
            /* Record in package database */
            InstalledApp *app = installed_app_new();
            app->name         = strdup(pd->task->app_name);
            app->full_name    = strdup(pd->task->full_name ?: pd->task->app_name);
            app->version      = strdup(pd->task->version ?: "unknown");
            app->install_path = strdup(p->install_path);
            app->install_type = strdup(installer_type_name(
                                    installer_detect_type(pd->task->filename)));
            app->repo_url     = strdup(pd->task->repo_url ?: "");
            app->installed_at = strdup("now");
            package_db_add(app);
        }

        g_source_remove(pd->timer_id);
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void on_close_dialog(GtkButton *btn, gpointer data) {
    (void)btn;
    ProgressDialog *pd = (ProgressDialog *)data;
    gtk_window_destroy(pd->dialog);
    install_progress_free(pd->progress);
    install_task_free(pd->task);
    free(pd);
}

static void start_install(GtkWindow *parent, const char *asset_name,
                          const char *asset_url, const char *app_name,
                          const char *full_name, const char *version,
                          const char *repo_url) {
    AppSettings *settings = settings_load();

    InstallTask *task = install_task_new();
    task->url       = strdup(asset_url);
    task->filename  = strdup(asset_name);
    task->app_name  = strdup(app_name);
    task->full_name = strdup(full_name ?: app_name);
    task->version   = strdup(version ?: "unknown");
    task->repo_url  = strdup(repo_url ?: "");
    task->token     = settings->github_token ? strdup(settings->github_token) : NULL;
    task->progress  = install_progress_new();
    settings_free(settings);

    /* Build progress dialog */
    GtkWindow *dlg = GTK_WINDOW(gtk_window_new());
    gtk_window_set_title(dlg, "Installing…");
    gtk_window_set_default_size(dlg, 380, 140);
    gtk_window_set_modal(dlg, TRUE);
    if (parent) gtk_window_set_transient_for(dlg, parent);

    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(box), 20);
    gtk_widget_set_margin_bottom(GTK_WIDGET(box), 20);
    gtk_window_set_child(dlg, GTK_WIDGET(box));

    GtkLabel *lbl = GTK_LABEL(gtk_label_new(asset_name));
    gtk_box_append(box, GTK_WIDGET(lbl));

    GtkProgressBar *bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_progress_bar_set_show_text(bar, TRUE);
    gtk_box_append(box, GTK_WIDGET(bar));

    GtkLabel *status = GTK_LABEL(gtk_label_new("Preparing…"));
    gtk_box_append(box, GTK_WIDGET(status));

    GtkButton *close_btn = GTK_BUTTON(gtk_button_new_with_label("Close"));
    gtk_widget_set_sensitive(GTK_WIDGET(close_btn), FALSE);
    gtk_box_append(box, GTK_WIDGET(close_btn));

    ProgressDialog *pd = malloc(sizeof(ProgressDialog));
    pd->dialog      = dlg;
    pd->bar         = bar;
    pd->status_label = status;
    pd->close_btn   = close_btn;
    pd->progress    = task->progress;
    pd->task        = task;

    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_dialog), pd);

    pd->timer_id = g_timeout_add(250, poll_progress, pd);

    gtk_window_present(dlg);

    /* Launch download+install in background thread */
    g_thread_new("installer", installer_run, task);
}

/* ── asset button ─────────────────────────────────────────── */

typedef struct {
    char *asset_name;
    char *asset_url;
    char *app_name;
    char *full_name;
    char *version;
    char *repo_url;
    GtkWindow *parent_window;
} AssetData;

static void asset_data_free(gpointer data) {
    AssetData *d = (AssetData *)data;
    free(d->asset_name);
    free(d->asset_url);
    free(d->app_name);
    free(d->full_name);
    free(d->version);
    free(d->repo_url);
    free(d);
}

static void on_install_asset(GtkButton *btn, gpointer data) {
    (void)btn;
    AssetData *d = (AssetData *)data;
    start_install(d->parent_window, d->asset_name, d->asset_url,
                  d->app_name, d->full_name, d->version, d->repo_url);
}

/* ── detail page builder ──────────────────────────────────── */

static void load_releases(PageHome *page) {
    /* clear existing rows */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->releases_list))))
        gtk_list_box_remove(page->releases_list, child);

    if (!page->current_full_name) return;

    AppSettings *settings = settings_load();
    GitHubService *svc = github_service_new(settings->github_token);
    settings_free(settings);

    int count = 0;
    GitHubRelease **releases = github_get_releases(svc, page->current_full_name, &count);
    github_service_free(svc);

    if (!releases || count == 0) {
        gtk_label_set_text(page->releases_hint, "No releases found for this repository.");
        return;
    }

    gtk_label_set_text(page->releases_hint, "");

    /* Find the parent window once */
    GtkWindow *parent_win = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(page->box)));

    for (int i = 0; i < count; i++) {
        GitHubRelease *rel = releases[i];

        GtkBox *rel_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
        gtk_widget_set_margin_start(GTK_WIDGET(rel_box), 12);
        gtk_widget_set_margin_end(GTK_WIDGET(rel_box), 12);
        gtk_widget_set_margin_top(GTK_WIDGET(rel_box), 10);
        gtk_widget_set_margin_bottom(GTK_WIDGET(rel_box), 10);

        /* Release header */
        char header[256];
        snprintf(header, sizeof(header), "<b>%s</b>  <small>%s</small>",
                 rel->tag_name ?: "?",
                 rel->published_at ? rel->published_at : "");
        GtkLabel *rel_lbl = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(rel_lbl, header);
        gtk_label_set_xalign(rel_lbl, 0.0f);
        gtk_box_append(rel_box, GTK_WIDGET(rel_lbl));

        if (rel->asset_count == 0) {
            GtkLabel *no_assets = GTK_LABEL(gtk_label_new("  No downloadable assets"));
            gtk_label_set_xalign(no_assets, 0.0f);
            gtk_box_append(rel_box, GTK_WIDGET(no_assets));
        }

        for (int j = 0; j < rel->asset_count; j++) {
            ReleaseAsset *asset = rel->assets[j];
            if (!asset->name || !asset->download_url) continue;

            GtkBox *asset_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
            gtk_widget_set_margin_start(GTK_WIDGET(asset_row), 12);

            /* Size label */
            char size_str[32] = "";
            if (asset->size > 0) {
                if (asset->size > 1024*1024)
                    snprintf(size_str, sizeof(size_str), " (%.1f MB)", asset->size / 1048576.0);
                else
                    snprintf(size_str, sizeof(size_str), " (%ld KB)", asset->size / 1024);
            }
            char asset_label[256];
            snprintf(asset_label, sizeof(asset_label), "%s%s", asset->name, size_str);

            GtkLabel *a_lbl = GTK_LABEL(gtk_label_new(asset_label));
            gtk_label_set_xalign(a_lbl, 0.0f);
            gtk_widget_set_hexpand(GTK_WIDGET(a_lbl), TRUE);
            gtk_box_append(asset_row, GTK_WIDGET(a_lbl));

            GtkButton *install_btn = GTK_BUTTON(gtk_button_new_with_label("Install"));
            gtk_widget_add_css_class(GTK_WIDGET(install_btn), "suggested-action");

            AssetData *ad = malloc(sizeof(AssetData));
            ad->asset_name    = strdup(asset->name);
            ad->asset_url     = strdup(asset->download_url);
            ad->app_name      = strdup(page->current_app_name ?: page->current_full_name);
            ad->full_name     = strdup(page->current_full_name);
            ad->version       = strdup(rel->tag_name ?: "unknown");
            ad->repo_url      = strdup(page->current_full_name);
            ad->parent_window = parent_win;

            g_signal_connect_data(install_btn, "clicked",
                                  G_CALLBACK(on_install_asset), ad,
                                  (GClosureNotify)asset_data_free, 0);
            gtk_box_append(asset_row, GTK_WIDGET(install_btn));
            gtk_box_append(rel_box, GTK_WIDGET(asset_row));
        }

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_selectable(row, FALSE);
        gtk_list_box_row_set_child(row, GTK_WIDGET(rel_box));
        gtk_list_box_append(page->releases_list, GTK_WIDGET(row));

        release_free(rel);
    }

    free(releases);
}

/* ── search result click ──────────────────────────────────── */

typedef struct {
    PageHome *page;
    char     *full_name;
    char     *app_name;
    char     *description;
    int       stars;
    int       forks;
    char     *language;
    char     *url;
} ResultData;

static void result_data_free(gpointer data) {
    ResultData *d = (ResultData *)data;
    free(d->full_name);
    free(d->app_name);
    free(d->description);
    free(d->language);
    free(d->url);
    free(d);
}

static void on_result_clicked(GtkListBox *list, GtkListBoxRow *row, gpointer data) {
    (void)list;
    ResultData *d = g_object_get_data(G_OBJECT(row), "result-data");
    if (!d) return;

    PageHome *page = d->page;

    /* Update detail labels */
    char name_markup[256];
    snprintf(name_markup, sizeof(name_markup), "<span size='large'><b>%s</b></span>",
             d->full_name ?: "");
    gtk_label_set_markup(page->detail_name, name_markup);

    char meta[256];
    snprintf(meta, sizeof(meta), "⭐ %d   🍴 %d   %s",
             d->stars, d->forks, d->language ?: "");
    gtk_label_set_text(page->detail_meta, meta);
    gtk_label_set_text(page->detail_desc, d->description ?: "");
    gtk_label_set_text(page->releases_hint, "Loading releases…");

    /* Store current app for install */
    free(page->current_full_name);
    free(page->current_app_name);
    page->current_full_name = strdup(d->full_name);
    page->current_app_name  = d->app_name ? strdup(d->app_name) : NULL;

    gtk_stack_set_visible_child_name(page->stack, "detail");
    load_releases(page);
}

/* ── back button ──────────────────────────────────────────── */

static void on_back_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    PageHome *page = (PageHome *)data;
    gtk_stack_set_visible_child_name(page->stack, "search");
}

/* ── search ───────────────────────────────────────────────── */

static void on_search_changed(GtkSearchEntry *entry, gpointer data) {
    PageHome *page = (PageHome *)data;
    const char *query = gtk_editable_get_text(GTK_EDITABLE(entry));

    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->results_list))))
        gtk_list_box_remove(page->results_list, child);

    if (!query || strlen(query) < 2) {
        gtk_label_set_text(page->results_hint, "Type at least 2 characters to search.");
        return;
    }

    gtk_label_set_text(page->results_hint, "Searching…");

    AppSettings *settings = settings_load();
    GitHubService *svc = github_service_new(settings->github_token);
    settings_free(settings);

    int count = 0;
    GitHubRepository **repos = github_search_repositories(svc, query, &count);
    github_service_free(svc);

    if (!repos || count == 0) {
        gtk_label_set_text(page->results_hint, "No results found.");
        free(repos);
        return;
    }

    gtk_label_set_text(page->results_hint, "");

    for (int i = 0; i < count; i++) {
        GitHubRepository *r = repos[i];

        GtkBox *item = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
        gtk_widget_set_margin_start(GTK_WIDGET(item), 12);
        gtk_widget_set_margin_end(GTK_WIDGET(item), 12);
        gtk_widget_set_margin_top(GTK_WIDGET(item), 10);
        gtk_widget_set_margin_bottom(GTK_WIDGET(item), 10);

        char title_markup[256];
        snprintf(title_markup, sizeof(title_markup),
                 "<b>%s</b>   <small>⭐ %d</small>", r->full_name ?: r->name ?: "", r->stars);
        GtkLabel *title_lbl = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(title_lbl, title_markup);
        gtk_label_set_xalign(title_lbl, 0.0f);
        gtk_box_append(item, GTK_WIDGET(title_lbl));

        if (r->description && strlen(r->description) > 0) {
            GtkLabel *desc_lbl = GTK_LABEL(gtk_label_new(r->description));
            gtk_label_set_xalign(desc_lbl, 0.0f);
            gtk_label_set_wrap(desc_lbl, TRUE);
            gtk_label_set_lines(desc_lbl, 2);
            gtk_label_set_ellipsize(desc_lbl, PANGO_ELLIPSIZE_END);
            gtk_box_append(item, GTK_WIDGET(desc_lbl));
        }

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_child(row, GTK_WIDGET(item));

        ResultData *rd = malloc(sizeof(ResultData));
        rd->page        = page;
        rd->full_name   = r->full_name  ? strdup(r->full_name)  : NULL;
        rd->app_name    = r->name       ? strdup(r->name)        : NULL;
        rd->description = r->description? strdup(r->description) : NULL;
        rd->stars       = r->stars;
        rd->forks       = r->forks;
        rd->language    = r->language   ? strdup(r->language)    : NULL;
        rd->url         = r->url        ? strdup(r->url)         : NULL;
        g_object_set_data_full(G_OBJECT(row), "result-data", rd,
                               (GDestroyNotify)result_data_free);

        gtk_list_box_append(page->results_list, GTK_WIDGET(row));
        repository_free(r);
    }

    free(repos);
}

/* ── constructor ──────────────────────────────────────────── */

PageHome *page_home_new(void) {
    PageHome *page = calloc(1, sizeof(PageHome));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

    page->stack = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(page->stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_box_append(page->box, GTK_WIDGET(page->stack));
    gtk_widget_set_vexpand(GTK_WIDGET(page->stack), TRUE);

    /* ── SEARCH page ── */
    GtkBox *search_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(search_box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(search_box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(search_box), 20);

    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<span size='x-large'><b>Search Apps</b></span>");
    gtk_label_set_xalign(title, 0.0f);
    gtk_box_append(search_box, GTK_WIDGET(title));

    page->search_entry = GTK_SEARCH_ENTRY(gtk_search_entry_new());
    gtk_search_entry_set_placeholder_text(page->search_entry, "Search GitHub repositories…");
    gtk_box_append(search_box, GTK_WIDGET(page->search_entry));

    page->results_hint = GTK_LABEL(gtk_label_new("Type to search GitHub repositories."));
    gtk_label_set_xalign(page->results_hint, 0.0f);
    gtk_box_append(search_box, GTK_WIDGET(page->results_hint));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);
    page->results_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_activate_on_single_click(page->results_list, TRUE);
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(page->results_list));
    gtk_box_append(search_box, GTK_WIDGET(scroll));

    gtk_stack_add_named(page->stack, GTK_WIDGET(search_box), "search");

    /* ── DETAIL page ── */
    GtkBox *detail_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(detail_box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(detail_box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(detail_box), 20);

    GtkButton *back_btn = GTK_BUTTON(gtk_button_new_with_label("← Back to Search"));
    gtk_widget_set_halign(GTK_WIDGET(back_btn), GTK_ALIGN_START);
    gtk_box_append(detail_box, GTK_WIDGET(back_btn));
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), page);

    page->detail_name = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->detail_name, 0.0f);
    gtk_label_set_wrap(page->detail_name, TRUE);
    gtk_box_append(detail_box, GTK_WIDGET(page->detail_name));

    page->detail_meta = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->detail_meta, 0.0f);
    gtk_box_append(detail_box, GTK_WIDGET(page->detail_meta));

    page->detail_desc = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->detail_desc, 0.0f);
    gtk_label_set_wrap(page->detail_desc, TRUE);
    gtk_box_append(detail_box, GTK_WIDGET(page->detail_desc));

    GtkLabel *releases_title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(releases_title, "<b>Releases</b>");
    gtk_label_set_xalign(releases_title, 0.0f);
    gtk_box_append(detail_box, GTK_WIDGET(releases_title));

    page->releases_hint = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->releases_hint, 0.0f);
    gtk_box_append(detail_box, GTK_WIDGET(page->releases_hint));

    GtkScrolledWindow *rel_scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(rel_scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(rel_scroll), TRUE);
    page->releases_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_activate_on_single_click(page->releases_list, FALSE);
    gtk_scrolled_window_set_child(rel_scroll, GTK_WIDGET(page->releases_list));
    gtk_box_append(detail_box, GTK_WIDGET(rel_scroll));

    gtk_stack_add_named(page->stack, GTK_WIDGET(detail_box), "detail");

    /* signals */
    g_signal_connect(page->search_entry, "search-changed",
                     G_CALLBACK(on_search_changed), page);
    g_signal_connect(page->results_list, "row-activated",
                     G_CALLBACK(on_result_clicked), page);

    gtk_stack_set_visible_child_name(page->stack, "search");
    return page;
}
