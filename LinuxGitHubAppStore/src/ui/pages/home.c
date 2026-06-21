#include "home.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include "../../services/installer.h"
#include "../../services/package_db.h"
#include "../../models/release.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/* ── helpers ──────────────────────────────────────────────── */

static GtkBox *make_section_header(const char *text) {
    GtkLabel *lbl = GTK_LABEL(gtk_label_new(text));
    gtk_widget_add_css_class(GTK_WIDGET(lbl), "section-header");
    gtk_label_set_xalign(lbl, 0.0f);
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_box_append(box, GTK_WIDGET(lbl));
    return box;
}

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

    if (!p->done) return G_SOURCE_CONTINUE;

    gtk_progress_bar_set_fraction(pd->bar, 1.0);
    gtk_widget_set_sensitive(GTK_WIDGET(pd->close_btn), TRUE);

    if (p->success && p->install_path) {
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

        gtk_widget_add_css_class(GTK_WIDGET(pd->status_label), "success");
    } else {
        gtk_widget_add_css_class(GTK_WIDGET(pd->status_label), "error");
    }

    g_source_remove(pd->timer_id);
    return G_SOURCE_REMOVE;
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

    GtkWindow *dlg = GTK_WINDOW(gtk_window_new());
    char dlg_title[128];
    snprintf(dlg_title, sizeof(dlg_title), "Installing %s", app_name);
    gtk_window_set_title(dlg, dlg_title);
    gtk_window_set_default_size(dlg, 400, 160);
    gtk_window_set_modal(dlg, TRUE);
    if (parent) gtk_window_set_transient_for(dlg, parent);
    gtk_window_set_resizable(dlg, FALSE);

    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 12));
    gtk_widget_set_margin_start(GTK_WIDGET(box), 24);
    gtk_widget_set_margin_end(GTK_WIDGET(box), 24);
    gtk_widget_set_margin_top(GTK_WIDGET(box), 20);
    gtk_widget_set_margin_bottom(GTK_WIDGET(box), 20);
    gtk_window_set_child(dlg, GTK_WIDGET(box));

    char hdr[256];
    snprintf(hdr, sizeof(hdr), "<b>%s</b>  <small>%s</small>",
             app_name, asset_name);
    GtkLabel *hdr_lbl = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(hdr_lbl, hdr);
    gtk_label_set_xalign(hdr_lbl, 0.0f);
    gtk_box_append(box, GTK_WIDGET(hdr_lbl));

    GtkProgressBar *bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_progress_bar_set_show_text(bar, TRUE);
    gtk_progress_bar_set_text(bar, "Starting…");
    gtk_box_append(box, GTK_WIDGET(bar));

    GtkLabel *status = GTK_LABEL(gtk_label_new("Preparing download…"));
    gtk_label_set_xalign(status, 0.0f);
    gtk_box_append(box, GTK_WIDGET(status));

    GtkButton *close_btn = GTK_BUTTON(gtk_button_new_with_label("Close"));
    gtk_widget_set_sensitive(GTK_WIDGET(close_btn), FALSE);
    gtk_widget_set_halign(GTK_WIDGET(close_btn), GTK_ALIGN_END);
    gtk_box_append(box, GTK_WIDGET(close_btn));

    ProgressDialog *pd = malloc(sizeof(ProgressDialog));
    pd->dialog       = dlg;
    pd->bar          = bar;
    pd->status_label = status;
    pd->close_btn    = close_btn;
    pd->progress     = task->progress;
    pd->task         = task;

    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_dialog), pd);
    pd->timer_id = g_timeout_add(200, poll_progress, pd);
    gtk_window_present(dlg);

    g_thread_new("installer", installer_run, task);
}

/* ── asset install button ─────────────────────────────────── */

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
    free(d->asset_name); free(d->asset_url);
    free(d->app_name);   free(d->full_name);
    free(d->version);    free(d->repo_url);
    free(d);
}

static void on_install_asset(GtkButton *btn, gpointer data) {
    (void)btn;
    AssetData *d = (AssetData *)data;
    start_install(d->parent_window, d->asset_name, d->asset_url,
                  d->app_name, d->full_name, d->version, d->repo_url);
}

/* ── releases loader ──────────────────────────────────────── */

static void load_releases(PageHome *page) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->releases_list))))
        gtk_list_box_remove(page->releases_list, child);

    if (!page->current_full_name) return;

    gtk_widget_set_visible(GTK_WIDGET(page->rel_spinner), TRUE);
    gtk_spinner_start(page->rel_spinner);
    gtk_label_set_text(page->releases_hint, "Loading releases…");

    AppSettings *settings = settings_load();
    GitHubService *svc = github_service_new(settings->github_token);
    settings_free(settings);

    int count = 0;
    GitHubRelease **releases = github_get_releases(svc, page->current_full_name, &count);
    github_service_free(svc);

    gtk_spinner_stop(page->rel_spinner);
    gtk_widget_set_visible(GTK_WIDGET(page->rel_spinner), FALSE);

    if (!releases || count == 0) {
        gtk_label_set_text(page->releases_hint,
                           "No releases found. This repository may not publish releases.");
        return;
    }
    gtk_label_set_text(page->releases_hint, "");

    GtkWindow *parent_win = GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(page->box)));

    for (int i = 0; i < count; i++) {
        GitHubRelease *rel = releases[i];

        GtkBox *rel_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
        gtk_widget_set_margin_start(GTK_WIDGET(rel_box), 14);
        gtk_widget_set_margin_end(GTK_WIDGET(rel_box), 14);
        gtk_widget_set_margin_top(GTK_WIDGET(rel_box), 10);
        gtk_widget_set_margin_bottom(GTK_WIDGET(rel_box), 10);

        /* Release header row */
        GtkBox *hdr_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
        char ver_markup[256];
        snprintf(ver_markup, sizeof(ver_markup),
                 "<b>%s</b>", rel->tag_name ?: "?");
        GtkLabel *ver_lbl = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(ver_lbl, ver_markup);
        gtk_label_set_xalign(ver_lbl, 0.0f);
        gtk_widget_set_hexpand(GTK_WIDGET(ver_lbl), TRUE);
        gtk_box_append(hdr_row, GTK_WIDGET(ver_lbl));

        if (rel->published_at) {
            /* show just the date part (first 10 chars of ISO8601) */
            char date[16] = {0};
            strncpy(date, rel->published_at, 10);
            GtkLabel *date_lbl = GTK_LABEL(gtk_label_new(date));
            gtk_widget_add_css_class(GTK_WIDGET(date_lbl), "muted");
            gtk_box_append(hdr_row, GTK_WIDGET(date_lbl));
        }
        gtk_box_append(rel_box, GTK_WIDGET(hdr_row));

        if (rel->asset_count == 0) {
            GtkLabel *no_a = GTK_LABEL(gtk_label_new("No downloadable assets for this release."));
            gtk_widget_add_css_class(GTK_WIDGET(no_a), "muted");
            gtk_label_set_xalign(no_a, 0.0f);
            gtk_widget_set_margin_start(GTK_WIDGET(no_a), 4);
            gtk_box_append(rel_box, GTK_WIDGET(no_a));
        }

        /* Per-asset rows with colored type badges */
        for (int j = 0; j < rel->asset_count; j++) {
            ReleaseAsset *asset = rel->assets[j];
            if (!asset->name || !asset->download_url) continue;

            InstallType t = installer_detect_type(asset->name);
            const char *badge_text  = NULL;
            const char *badge_class = NULL;
            if      (t == INSTALL_TYPE_APPIMAGE) { badge_text = "AppImage"; badge_class = "badge-appimage"; }
            else if (t == INSTALL_TYPE_DEB)      { badge_text = "deb";      badge_class = "badge-deb"; }
            else if (t == INSTALL_TYPE_TARBALL)  { badge_text = "tar";      badge_class = "badge-tar"; }
            else if (t == INSTALL_TYPE_ZIP)      { badge_text = "zip";      badge_class = "badge-zip"; }
            else if (t == INSTALL_TYPE_RPM)      { badge_text = "rpm";      badge_class = "badge-rpm"; }

            GtkBox *asset_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
            gtk_widget_set_margin_start(GTK_WIDGET(asset_row), 8);
            gtk_widget_set_margin_top(GTK_WIDGET(asset_row), 3);
            gtk_widget_set_margin_bottom(GTK_WIDGET(asset_row), 3);
            gtk_widget_set_margin_end(GTK_WIDGET(asset_row), 8);

            GtkLabel *a_lbl = GTK_LABEL(gtk_label_new(asset->name));
            gtk_label_set_xalign(a_lbl, 0.0f);
            gtk_label_set_ellipsize(a_lbl, PANGO_ELLIPSIZE_MIDDLE);
            gtk_widget_set_hexpand(GTK_WIDGET(a_lbl), TRUE);
            gtk_box_append(asset_row, GTK_WIDGET(a_lbl));

            if (badge_text) {
                GtkLabel *badge = GTK_LABEL(gtk_label_new(badge_text));
                gtk_widget_add_css_class(GTK_WIDGET(badge), badge_class);
                gtk_widget_set_valign(GTK_WIDGET(badge), GTK_ALIGN_CENTER);
                gtk_box_append(asset_row, GTK_WIDGET(badge));
            }

            if (asset->size > 0) {
                char size_str[24];
                if (asset->size >= 1024*1024)
                    snprintf(size_str, sizeof(size_str), "%.1f MB", asset->size/1048576.0);
                else
                    snprintf(size_str, sizeof(size_str), "%ld KB", asset->size/1024);
                GtkLabel *sz = GTK_LABEL(gtk_label_new(size_str));
                gtk_widget_add_css_class(GTK_WIDGET(sz), "muted");
                gtk_widget_set_valign(GTK_WIDGET(sz), GTK_ALIGN_CENTER);
                gtk_box_append(asset_row, GTK_WIDGET(sz));
            }

            GtkButton *install_btn = GTK_BUTTON(gtk_button_new_with_label("Install"));
            gtk_widget_add_css_class(GTK_WIDGET(install_btn), "suggested-action");

            AssetData *ad = malloc(sizeof(AssetData));
            ad->asset_name    = strdup(asset->name);
            ad->asset_url     = strdup(asset->download_url);
            ad->app_name      = strdup(page->current_app_name ?: page->current_full_name ?: "app");
            ad->full_name     = strdup(page->current_full_name ?: "");
            ad->version       = strdup(rel->tag_name ?: "unknown");
            ad->repo_url      = strdup(page->current_full_name ?: "");
            ad->parent_window = parent_win;

            g_signal_connect_data(install_btn, "clicked",
                                  G_CALLBACK(on_install_asset), ad,
                                  (GClosureNotify)asset_data_free, 0);
            gtk_box_append(asset_row, GTK_WIDGET(install_btn));
            gtk_box_append(rel_box, GTK_WIDGET(asset_row));
        }

        /* Add separator between releases */
        if (i < count - 1) {
            GtkSeparator *s = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
            gtk_widget_set_margin_top(GTK_WIDGET(s), 4);
            gtk_box_append(rel_box, GTK_WIDGET(s));
        }

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_selectable(row, FALSE);
        gtk_list_box_row_set_child(row, GTK_WIDGET(rel_box));
        gtk_list_box_append(page->releases_list, GTK_WIDGET(row));

        release_free(rel);
    }
    free(releases);
}

/* ── result click → detail ────────────────────────────────── */

typedef struct {
    PageHome *page;
    char *full_name;
    char *app_name;
    char *description;
    int   stars;
    int   forks;
    char *language;
    char *url;
} ResultData;

static void result_data_free(gpointer data) {
    ResultData *d = (ResultData *)data;
    free(d->full_name); free(d->app_name);
    free(d->description); free(d->language); free(d->url);
    free(d);
}

static void on_result_activated(GtkListBox *list, GtkListBoxRow *row, gpointer data) {
    (void)list;
    ResultData *d = g_object_get_data(G_OBJECT(row), "result-data");
    if (!d) return;
    PageHome *page = d->page;

    char name_markup[256];
    snprintf(name_markup, sizeof(name_markup),
             "<span size='large'><b>%s</b></span>", d->full_name ?: "");
    gtk_label_set_markup(page->detail_name, name_markup);

    char meta[256];
    snprintf(meta, sizeof(meta), "⭐ %d stars   🍴 %d forks   %s",
             d->stars, d->forks, d->language ?: "");
    gtk_label_set_text(page->detail_meta, meta);
    gtk_label_set_text(page->detail_desc, d->description ?: "No description.");

    free(page->current_full_name);
    free(page->current_app_name);
    page->current_full_name = d->full_name ? strdup(d->full_name) : NULL;
    page->current_app_name  = d->app_name  ? strdup(d->app_name)  : NULL;

    gtk_stack_set_visible_child_name(page->stack, "detail");
    load_releases(page);
}

/* ── search ───────────────────────────────────────────────── */

static void do_search(PageHome *page, const char *query) {
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->results_list))))
        gtk_list_box_remove(page->results_list, child);

    gtk_widget_set_visible(GTK_WIDGET(page->search_spinner), TRUE);
    gtk_spinner_start(page->search_spinner);
    gtk_label_set_text(page->results_hint, "Searching GitHub…");
    gtk_widget_set_visible(GTK_WIDGET(page->featured_box), FALSE);

    AppSettings *settings = settings_load();
    GitHubService *svc = github_service_new(settings->github_token);
    settings_free(settings);

    int count = 0;
    GitHubRepository **repos = github_search_repositories(svc, query, &count);
    github_service_free(svc);

    gtk_spinner_stop(page->search_spinner);
    gtk_widget_set_visible(GTK_WIDGET(page->search_spinner), FALSE);

    if (!repos || count == 0) {
        gtk_label_set_markup(page->results_hint,
            "<span color='#dc2626'>No results found.</span>  Try a different search term.");
        free(repos);
        return;
    }

    char hint_text[64];
    snprintf(hint_text, sizeof(hint_text), "Found %d repositories — click to view releases", count);
    gtk_label_set_text(page->results_hint, hint_text);

    for (int i = 0; i < count; i++) {
        GitHubRepository *r = repos[i];

        GtkBox *item = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
        gtk_widget_set_margin_start(GTK_WIDGET(item), 14);
        gtk_widget_set_margin_end(GTK_WIDGET(item), 14);
        gtk_widget_set_margin_top(GTK_WIDGET(item), 10);
        gtk_widget_set_margin_bottom(GTK_WIDGET(item), 10);

        GtkBox *top_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6));

        char markup[256];
        snprintf(markup, sizeof(markup), "<b>%s</b>", r->full_name ?: r->name ?: "");
        GtkLabel *name_lbl = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(name_lbl, markup);
        gtk_label_set_xalign(name_lbl, 0.0f);
        gtk_widget_set_hexpand(GTK_WIDGET(name_lbl), TRUE);
        gtk_box_append(top_row, GTK_WIDGET(name_lbl));

        char stars[32];
        snprintf(stars, sizeof(stars), "⭐ %d", r->stars);
        GtkLabel *stars_lbl = GTK_LABEL(gtk_label_new(stars));
        gtk_widget_add_css_class(GTK_WIDGET(stars_lbl), "muted");
        gtk_box_append(top_row, GTK_WIDGET(stars_lbl));

        gtk_box_append(item, GTK_WIDGET(top_row));

        if (r->description && strlen(r->description) > 0) {
            GtkLabel *desc = GTK_LABEL(gtk_label_new(r->description));
            gtk_label_set_xalign(desc, 0.0f);
            gtk_label_set_wrap(desc, TRUE);
            gtk_label_set_lines(desc, 2);
            gtk_label_set_ellipsize(desc, PANGO_ELLIPSIZE_END);
            gtk_widget_add_css_class(GTK_WIDGET(desc), "muted");
            gtk_box_append(item, GTK_WIDGET(desc));
        }

        /* Bottom row: language + forks */
        GtkBox *bottom_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
        gtk_widget_set_margin_top(GTK_WIDGET(bottom_row), 4);

        if (r->language && strlen(r->language) > 0) {
            GtkLabel *lang = GTK_LABEL(gtk_label_new(r->language));
            gtk_label_set_xalign(lang, 0.0f);
            gtk_widget_add_css_class(GTK_WIDGET(lang), "badge-lang");
            gtk_widget_set_valign(GTK_WIDGET(lang), GTK_ALIGN_CENTER);
            gtk_box_append(bottom_row, GTK_WIDGET(lang));
        }

        if (r->forks > 0) {
            char forks_str[32];
            snprintf(forks_str, sizeof(forks_str), "🍴 %d", r->forks);
            GtkLabel *forks_lbl = GTK_LABEL(gtk_label_new(forks_str));
            gtk_widget_add_css_class(GTK_WIDGET(forks_lbl), "muted");
            gtk_widget_set_valign(GTK_WIDGET(forks_lbl), GTK_ALIGN_CENTER);
            gtk_box_append(bottom_row, GTK_WIDGET(forks_lbl));
        }

        gtk_box_append(item, GTK_WIDGET(bottom_row));

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_child(row, GTK_WIDGET(item));

        ResultData *rd = malloc(sizeof(ResultData));
        rd->page        = page;
        rd->full_name   = r->full_name   ? strdup(r->full_name)   : NULL;
        rd->app_name    = r->name        ? strdup(r->name)         : NULL;
        rd->description = r->description ? strdup(r->description)  : NULL;
        rd->stars       = r->stars;
        rd->forks       = r->forks;
        rd->language    = r->language    ? strdup(r->language)     : NULL;
        rd->url         = r->url         ? strdup(r->url)          : NULL;
        g_object_set_data_full(G_OBJECT(row), "result-data", rd,
                               (GDestroyNotify)result_data_free);

        gtk_list_box_append(page->results_list, GTK_WIDGET(row));
        repository_free(r);
    }
    free(repos);
}

static void on_search_changed(GtkSearchEntry *entry, gpointer data) {
    PageHome *page = (PageHome *)data;
    const char *query = gtk_editable_get_text(GTK_EDITABLE(entry));

    if (!query || strlen(query) < 2) {
        GtkWidget *child;
        while ((child = gtk_widget_get_first_child(GTK_WIDGET(page->results_list))))
            gtk_list_box_remove(page->results_list, child);
        gtk_label_set_text(page->results_hint, "");
        gtk_widget_set_visible(GTK_WIDGET(page->featured_box), TRUE);
        return;
    }

    do_search(page, query);
}

static void on_search_activate(GtkSearchEntry *entry, gpointer data) {
    PageHome *page = (PageHome *)data;
    const char *query = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (query && strlen(query) >= 2) do_search(page, query);
}

/* ── category chip click ──────────────────────────────────── */

static void on_category_clicked(GtkButton *btn, gpointer data) {
    PageHome *page = (PageHome *)data;
    const char *term = gtk_button_get_label(btn);
    gtk_editable_set_text(GTK_EDITABLE(page->search_entry), term);
    do_search(page, term);
}

/* ── back button ──────────────────────────────────────────── */

static void on_back_clicked(GtkButton *btn, gpointer data) {
    (void)btn;
    PageHome *page = (PageHome *)data;
    gtk_stack_set_visible_child_name(page->stack, "search");
}

/* ── constructor ──────────────────────────────────────────── */

PageHome *page_home_new(void) {
    PageHome *page = calloc(1, sizeof(PageHome));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

    page->stack = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(page->stack, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(page->stack, 200);
    gtk_box_append(page->box, GTK_WIDGET(page->stack));
    gtk_widget_set_vexpand(GTK_WIDGET(page->stack), TRUE);

    /* ═══════════════ SEARCH PAGE ═══════════════ */
    GtkBox *search_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(search_box), 24);
    gtk_widget_set_margin_end(GTK_WIDGET(search_box), 24);
    gtk_widget_set_margin_top(GTK_WIDGET(search_box), 24);

    /* Title row */
    GtkBox *title_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
    GtkLabel *title = GTK_LABEL(gtk_label_new("Search Apps"));
    gtk_widget_add_css_class(GTK_WIDGET(title), "page-title");
    gtk_label_set_xalign(title, 0.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(title), TRUE);
    gtk_box_append(title_row, GTK_WIDGET(title));

    page->search_spinner = GTK_SPINNER(gtk_spinner_new());
    gtk_widget_set_visible(GTK_WIDGET(page->search_spinner), FALSE);
    gtk_box_append(title_row, GTK_WIDGET(page->search_spinner));
    gtk_box_append(search_box, GTK_WIDGET(title_row));

    GtkLabel *sub = GTK_LABEL(gtk_label_new("Discover GitHub repositories and install apps"));
    gtk_widget_add_css_class(GTK_WIDGET(sub), "page-subtitle");
    gtk_label_set_xalign(sub, 0.0f);
    gtk_box_append(search_box, GTK_WIDGET(sub));

    page->search_entry = GTK_SEARCH_ENTRY(gtk_search_entry_new());
    gtk_search_entry_set_placeholder_text(page->search_entry,
                                          "Search GitHub repositories…  (press Enter)");
    gtk_widget_set_size_request(GTK_WIDGET(page->search_entry), -1, 42);
    gtk_box_append(search_box, GTK_WIDGET(page->search_entry));

    /* Featured categories */
    page->featured_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8));
    gtk_box_append(search_box, GTK_WIDGET(page->featured_box));

    gtk_box_append(page->featured_box, GTK_WIDGET(make_section_header("POPULAR CATEGORIES")));

    static const char *cats[] = {
        "text editor", "file manager", "terminal emulator",
        "media player", "image viewer", "system monitor",
        "browser", "code editor", "game", "download manager",
    };
    GtkFlowBox *chips = GTK_FLOW_BOX(gtk_flow_box_new());
    gtk_flow_box_set_selection_mode(chips, GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(chips, FALSE);
    gtk_flow_box_set_column_spacing(chips, 6);
    gtk_flow_box_set_row_spacing(chips, 6);
    gtk_flow_box_set_max_children_per_line(chips, 20);
    gtk_widget_set_halign(GTK_WIDGET(chips), GTK_ALIGN_START);
    gtk_box_append(page->featured_box, GTK_WIDGET(chips));

    for (size_t i = 0; i < sizeof(cats)/sizeof(cats[0]); i++) {
        GtkButton *chip = GTK_BUTTON(gtk_button_new_with_label(cats[i]));
        gtk_widget_add_css_class(GTK_WIDGET(chip), "chip");
        g_signal_connect(chip, "clicked", G_CALLBACK(on_category_clicked), page);
        gtk_flow_box_append(chips, GTK_WIDGET(chip));
    }

    page->results_hint = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->results_hint, 0.0f);
    gtk_label_set_wrap(page->results_hint, TRUE);
    gtk_box_append(search_box, GTK_WIDGET(page->results_hint));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);
    page->results_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_activate_on_single_click(page->results_list, TRUE);
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(page->results_list));
    gtk_box_append(search_box, GTK_WIDGET(scroll));

    gtk_stack_add_named(page->stack, GTK_WIDGET(search_box), "search");

    /* ═══════════════ DETAIL PAGE ═══════════════ */
    GtkBox *detail_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(detail_box), 24);
    gtk_widget_set_margin_end(GTK_WIDGET(detail_box), 24);
    gtk_widget_set_margin_top(GTK_WIDGET(detail_box), 24);

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
    gtk_widget_add_css_class(GTK_WIDGET(page->detail_meta), "muted");
    gtk_box_append(detail_box, GTK_WIDGET(page->detail_meta));

    page->detail_desc = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->detail_desc, 0.0f);
    gtk_label_set_wrap(page->detail_desc, TRUE);
    gtk_box_append(detail_box, GTK_WIDGET(page->detail_desc));

    GtkSeparator *sep = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(detail_box, GTK_WIDGET(sep));

    /* Releases header row */
    GtkBox *rel_hdr = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
    GtkLabel *rel_title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(rel_title, "<b>Releases</b>");
    gtk_label_set_xalign(rel_title, 0.0f);
    gtk_widget_set_hexpand(GTK_WIDGET(rel_title), TRUE);
    gtk_box_append(rel_hdr, GTK_WIDGET(rel_title));

    page->rel_spinner = GTK_SPINNER(gtk_spinner_new());
    gtk_widget_set_visible(GTK_WIDGET(page->rel_spinner), FALSE);
    gtk_box_append(rel_hdr, GTK_WIDGET(page->rel_spinner));
    gtk_box_append(detail_box, GTK_WIDGET(rel_hdr));

    page->releases_hint = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->releases_hint, 0.0f);
    gtk_widget_add_css_class(GTK_WIDGET(page->releases_hint), "muted");
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
    g_signal_connect(page->search_entry, "activate",
                     G_CALLBACK(on_search_activate), page);
    g_signal_connect(page->results_list, "row-activated",
                     G_CALLBACK(on_result_activated), page);

    gtk_stack_set_visible_child_name(page->stack, "search");
    return page;
}
