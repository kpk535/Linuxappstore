#include "settings.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include "../../services/package_db.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void save_token(PageSettings *page) {
    const char *token = gtk_editable_get_text(GTK_EDITABLE(page->token_entry));
    AppSettings *settings = settings_load();
    free(settings->github_token);
    settings->github_token = (token && strlen(token)) ? strdup(token) : NULL;
    settings_save(settings);
    settings_free(settings);
}

static void on_save(GtkButton *btn, gpointer data) {
    (void)btn;
    PageSettings *page = (PageSettings *)data;
    save_token(page);
    gtk_label_set_text(page->status_label, "Token saved.");
    gtk_widget_remove_css_class(GTK_WIDGET(page->status_label), "error");
    gtk_widget_add_css_class(GTK_WIDGET(page->status_label), "success");
}

static void on_test(GtkButton *btn, gpointer data) {
    (void)btn;
    PageSettings *page = (PageSettings *)data;
    const char *token = gtk_editable_get_text(GTK_EDITABLE(page->token_entry));

    if (!token || strlen(token) == 0) {
        gtk_label_set_text(page->status_label, "Enter a token first.");
        gtk_widget_add_css_class(GTK_WIDGET(page->status_label), "error");
        gtk_widget_remove_css_class(GTK_WIDGET(page->status_label), "success");
        return;
    }

    gtk_label_set_text(page->status_label, "Testing…");
    gtk_widget_remove_css_class(GTK_WIDGET(page->status_label), "error");
    gtk_widget_remove_css_class(GTK_WIDGET(page->status_label), "success");
    gtk_widget_set_sensitive(GTK_WIDGET(page->test_btn), FALSE);

    GitHubService *svc = github_service_new(token);
    char *result = github_validate_token(svc);
    github_service_free(svc);

    if (result && strcmp(result, "true") == 0) {
        gtk_label_set_text(page->status_label, "✓ Token is valid — saved.");
        gtk_widget_add_css_class(GTK_WIDGET(page->status_label), "success");
        gtk_widget_remove_css_class(GTK_WIDGET(page->status_label), "error");
        save_token(page);
    } else {
        gtk_label_set_text(page->status_label, "✗ Token is invalid. Check your token and try again.");
        gtk_widget_add_css_class(GTK_WIDGET(page->status_label), "error");
        gtk_widget_remove_css_class(GTK_WIDGET(page->status_label), "success");
    }

    free(result);
    gtk_widget_set_sensitive(GTK_WIDGET(page->test_btn), TRUE);
}

static void on_clear_cache(GtkButton *btn, gpointer data) {
    (void)btn;
    PageSettings *page = (PageSettings *)data;
    package_db_clear();
    gtk_label_set_text(page->cache_status, "Package database cleared.");
    gtk_widget_add_css_class(GTK_WIDGET(page->cache_status), "success");
}

static GtkBox *make_card(void) {
    GtkBox *card = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_add_css_class(GTK_WIDGET(card), "settings-card");
    return card;
}

static GtkWidget *card_title(const char *text) {
    GtkLabel *lbl = GTK_LABEL(gtk_label_new(text));
    gtk_widget_add_css_class(GTK_WIDGET(lbl), "settings-section-title");
    gtk_label_set_xalign(lbl, 0.0f);
    return GTK_WIDGET(lbl);
}

static GtkWidget *hint_box(const char *text) {
    GtkLabel *lbl = GTK_LABEL(gtk_label_new(text));
    gtk_widget_add_css_class(GTK_WIDGET(lbl), "info-box");
    gtk_label_set_xalign(lbl, 0.0f);
    gtk_label_set_wrap(lbl, TRUE);
    return GTK_WIDGET(lbl);
}

PageSettings *page_settings_new(void) {
    PageSettings *page = calloc(1, sizeof(PageSettings));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 14));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_bottom(GTK_WIDGET(page->box), 24);

    /* Page title */
    GtkLabel *title = GTK_LABEL(gtk_label_new("Settings"));
    gtk_widget_add_css_class(GTK_WIDGET(title), "page-title");
    gtk_label_set_xalign(title, 0.0f);
    gtk_box_append(page->box, GTK_WIDGET(title));

    GtkLabel *sub = GTK_LABEL(gtk_label_new("Configure your GitHub token and app preferences."));
    gtk_widget_add_css_class(GTK_WIDGET(sub), "page-subtitle");
    gtk_label_set_xalign(sub, 0.0f);
    gtk_box_append(page->box, GTK_WIDGET(sub));

    /* ── GitHub Token card ────────────────────────────── */
    GtkBox *tok_card = make_card();
    gtk_box_append(tok_card, card_title("GitHub Token"));
    gtk_box_append(tok_card, hint_box(
        "A personal access token lets the app search GitHub and check releases. "
        "Create one at GitHub → Settings → Developer settings → Personal access tokens."));

    page->token_entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_visibility(page->token_entry, FALSE);
    gtk_entry_set_placeholder_text(page->token_entry,
        "ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    gtk_widget_add_css_class(GTK_WIDGET(page->token_entry), "token-entry");
    gtk_box_append(tok_card, GTK_WIDGET(page->token_entry));

    GtkBox *btn_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
    page->test_btn = GTK_BUTTON(gtk_button_new_with_label("Test & Save Token"));
    gtk_widget_add_css_class(GTK_WIDGET(page->test_btn), "suggested-action");
    page->save_btn = GTK_BUTTON(gtk_button_new_with_label("Save"));
    gtk_widget_add_css_class(GTK_WIDGET(page->save_btn), "neutral-action");
    gtk_box_append(btn_row, GTK_WIDGET(page->test_btn));
    gtk_box_append(btn_row, GTK_WIDGET(page->save_btn));
    gtk_box_append(tok_card, GTK_WIDGET(btn_row));

    page->status_label = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->status_label, 0.0f);
    gtk_box_append(tok_card, GTK_WIDGET(page->status_label));

    gtk_box_append(page->box, GTK_WIDGET(tok_card));

    /* ── Data & Cache card ────────────────────────────── */
    GtkBox *cache_card = make_card();
    gtk_box_append(cache_card, card_title("Data & Cache"));
    gtk_box_append(cache_card, hint_box(
        "The package database tracks installed apps. "
        "Clearing it removes all install records but does not uninstall apps."));

    page->clear_cache_btn = GTK_BUTTON(gtk_button_new_with_label("Clear Package Database"));
    gtk_widget_add_css_class(GTK_WIDGET(page->clear_cache_btn), "destructive-action");
    gtk_widget_set_halign(GTK_WIDGET(page->clear_cache_btn), GTK_ALIGN_START);
    gtk_box_append(cache_card, GTK_WIDGET(page->clear_cache_btn));

    page->cache_status = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->cache_status, 0.0f);
    gtk_box_append(cache_card, GTK_WIDGET(page->cache_status));

    gtk_box_append(page->box, GTK_WIDGET(cache_card));

    /* ── About card ───────────────────────────────────── */
    GtkBox *about_card = make_card();
    gtk_box_append(about_card, card_title("About"));

    GtkLabel *about_name = GTK_LABEL(gtk_label_new("Linux GitHub App Store  v1.2.0"));
    gtk_widget_add_css_class(GTK_WIDGET(about_name), "settings-section-title");
    gtk_label_set_xalign(about_name, 0.0f);
    gtk_box_append(about_card, GTK_WIDGET(about_name));

    GtkLabel *about_desc = GTK_LABEL(gtk_label_new(
        "An open-source GTK4 app store for GitHub releases.\n"
        "Built with GTK4, libcurl, and json-c."));
    gtk_label_set_xalign(about_desc, 0.0f);
    gtk_label_set_wrap(about_desc, TRUE);
    gtk_widget_add_css_class(GTK_WIDGET(about_desc), "muted");
    gtk_box_append(about_card, GTK_WIDGET(about_desc));

    gtk_box_append(page->box, GTK_WIDGET(about_card));

    /* Spacer */
    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(page->box, GTK_WIDGET(spacer));

    /* Load existing token */
    AppSettings *settings = settings_load();
    if (settings->github_token)
        gtk_editable_set_text(GTK_EDITABLE(page->token_entry), settings->github_token);
    settings_free(settings);

    g_signal_connect(page->test_btn,        "clicked", G_CALLBACK(on_test),        page);
    g_signal_connect(page->save_btn,        "clicked", G_CALLBACK(on_save),        page);
    g_signal_connect(page->clear_cache_btn, "clicked", G_CALLBACK(on_clear_cache), page);

    return page;
}
