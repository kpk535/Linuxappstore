#include "profile.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <json-c/json.h>

void page_profile_refresh(PageProfile *page) {
    gtk_label_set_text(page->status_label, "Loading profile…");
    gtk_widget_set_sensitive(GTK_WIDGET(page->refresh_btn), FALSE);

    AppSettings *settings = settings_load();
    if (!settings->github_token || strlen(settings->github_token) == 0) {
        gtk_label_set_text(page->avatar_label, "?");
        gtk_label_set_text(page->login_label,  "Not signed in");
        gtk_label_set_text(page->name_label,   "Add a GitHub token in Settings");
        gtk_label_set_text(page->bio_label,    "");
        gtk_label_set_text(page->repos_val,     "-");
        gtk_label_set_text(page->followers_val, "-");
        gtk_label_set_text(page->following_val, "-");
        gtk_label_set_text(page->status_label,  "No GitHub token configured.");
        settings_free(settings);
        gtk_widget_set_sensitive(GTK_WIDGET(page->refresh_btn), TRUE);
        return;
    }

    GitHubService *svc = github_service_new(settings->github_token);
    settings_free(settings);

    char *json_str = github_get_user_profile(svc);
    github_service_free(svc);

    if (!json_str) {
        gtk_label_set_text(page->status_label, "Failed to fetch profile. Check your token.");
        gtk_widget_set_sensitive(GTK_WIDGET(page->refresh_btn), TRUE);
        return;
    }

    json_object *obj = json_tokener_parse(json_str);
    free(json_str);

    if (!obj) {
        gtk_label_set_text(page->status_label, "Invalid response from GitHub.");
        gtk_widget_set_sensitive(GTK_WIDGET(page->refresh_btn), TRUE);
        return;
    }

    json_object *j;
    const char *login = "", *name = "", *bio = "";
    int repos = 0, followers = 0, following = 0;

    if (json_object_object_get_ex(obj, "login",       &j)) login     = json_object_get_string(j);
    if (json_object_object_get_ex(obj, "name",        &j)) name      = json_object_get_string(j) ?: "";
    if (json_object_object_get_ex(obj, "bio",         &j)) bio       = json_object_get_string(j) ?: "";
    if (json_object_object_get_ex(obj, "public_repos",&j)) repos     = json_object_get_int(j);
    if (json_object_object_get_ex(obj, "followers",   &j)) followers = json_object_get_int(j);
    if (json_object_object_get_ex(obj, "following",   &j)) following = json_object_get_int(j);

    /* Update avatar initial */
    char init_ch[4];
    init_ch[0] = (login && login[0]) ? (char)g_ascii_toupper((guchar)login[0]) : '?';
    init_ch[1] = '\0';
    gtk_label_set_text(page->avatar_label, init_ch);

    char login_str[128];
    snprintf(login_str, sizeof(login_str), "@%s", login);
    gtk_label_set_text(page->login_label, login_str);
    gtk_label_set_text(page->name_label,  (name && strlen(name)) ? name : "");
    gtk_label_set_text(page->bio_label,   (bio  && strlen(bio))  ? bio  : "");

    char num[16];
    snprintf(num, sizeof(num), "%d", repos);     gtk_label_set_text(page->repos_val,     num);
    snprintf(num, sizeof(num), "%d", followers); gtk_label_set_text(page->followers_val, num);
    snprintf(num, sizeof(num), "%d", following); gtk_label_set_text(page->following_val, num);

    gtk_label_set_text(page->status_label, "");
    json_object_put(obj);
    gtk_widget_set_sensitive(GTK_WIDGET(page->refresh_btn), TRUE);
}

static void on_refresh(GtkButton *btn, gpointer data) {
    (void)btn;
    page_profile_refresh((PageProfile *)data);
}

static GtkWidget *stat_box(const char *icon, const char *label, GtkLabel **out_val) {
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
    gtk_widget_add_css_class(GTK_WIDGET(box), "stat-box");
    gtk_widget_set_hexpand(GTK_WIDGET(box), TRUE);

    GtkLabel *ico = GTK_LABEL(gtk_label_new(icon));
    gtk_widget_add_css_class(GTK_WIDGET(ico), "stat-icon");
    gtk_label_set_xalign(ico, 0.5f);
    gtk_box_append(box, GTK_WIDGET(ico));

    GtkLabel *val = GTK_LABEL(gtk_label_new("-"));
    gtk_widget_add_css_class(GTK_WIDGET(val), "stat-value");
    gtk_label_set_xalign(val, 0.5f);
    gtk_box_append(box, GTK_WIDGET(val));

    GtkLabel *lbl = GTK_LABEL(gtk_label_new(label));
    gtk_widget_add_css_class(GTK_WIDGET(lbl), "stat-label");
    gtk_label_set_xalign(lbl, 0.5f);
    gtk_box_append(box, GTK_WIDGET(lbl));

    if (out_val) *out_val = val;
    return GTK_WIDGET(box);
}

PageProfile *page_profile_new(void) {
    PageProfile *page = calloc(1, sizeof(PageProfile));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 16));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 24);
    gtk_widget_set_margin_bottom(GTK_WIDGET(page->box), 24);

    /* ── Gradient banner ─────────────────────────────── */
    page->banner = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 6));
    gtk_widget_add_css_class(GTK_WIDGET(page->banner), "profile-banner");

    /* Top row: avatar + info + refresh button */
    GtkBox *banner_top = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16));
    gtk_widget_set_valign(GTK_WIDGET(banner_top), GTK_ALIGN_CENTER);

    /* Avatar circle */
    page->avatar_label = GTK_LABEL(gtk_label_new("?"));
    gtk_widget_add_css_class(GTK_WIDGET(page->avatar_label), "profile-avatar-lg");
    gtk_label_set_xalign(page->avatar_label, 0.5f);
    gtk_label_set_yalign(page->avatar_label, 0.5f);
    gtk_widget_set_valign(GTK_WIDGET(page->avatar_label), GTK_ALIGN_CENTER);
    gtk_widget_set_halign(GTK_WIDGET(page->avatar_label), GTK_ALIGN_CENTER);
    gtk_box_append(banner_top, GTK_WIDGET(page->avatar_label));

    /* Name + login column */
    GtkBox *name_col = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
    gtk_widget_set_hexpand(GTK_WIDGET(name_col), TRUE);
    gtk_widget_set_valign(GTK_WIDGET(name_col), GTK_ALIGN_CENTER);

    page->login_label = GTK_LABEL(gtk_label_new("@—"));
    gtk_widget_add_css_class(GTK_WIDGET(page->login_label), "profile-login");
    gtk_label_set_xalign(page->login_label, 0.0f);
    gtk_box_append(name_col, GTK_WIDGET(page->login_label));

    page->name_label = GTK_LABEL(gtk_label_new(""));
    gtk_widget_add_css_class(GTK_WIDGET(page->name_label), "profile-name");
    gtk_label_set_xalign(page->name_label, 0.0f);
    gtk_box_append(name_col, GTK_WIDGET(page->name_label));

    gtk_box_append(banner_top, GTK_WIDGET(name_col));

    /* Refresh button top-right */
    page->refresh_btn = GTK_BUTTON(gtk_button_new_with_label("⟳  Refresh"));
    gtk_widget_add_css_class(GTK_WIDGET(page->refresh_btn), "profile-refresh-btn");
    gtk_widget_set_valign(GTK_WIDGET(page->refresh_btn), GTK_ALIGN_START);
    gtk_box_append(banner_top, GTK_WIDGET(page->refresh_btn));

    gtk_box_append(page->banner, GTK_WIDGET(banner_top));

    /* Bio below name row, inside banner */
    page->bio_label = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->bio_label, 0.0f);
    gtk_label_set_wrap(page->bio_label, TRUE);
    gtk_widget_add_css_class(GTK_WIDGET(page->bio_label), "profile-name");
    gtk_box_append(page->banner, GTK_WIDGET(page->bio_label));

    gtk_box_append(page->box, GTK_WIDGET(page->banner));

    /* ── Stats row ───────────────────────────────────── */
    GtkBox *stats = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_box_append(stats, stat_box("📁", "Public Repos", &page->repos_val));
    gtk_box_append(stats, stat_box("👥", "Followers",    &page->followers_val));
    gtk_box_append(stats, stat_box("👣", "Following",    &page->following_val));
    gtk_box_append(page->box, GTK_WIDGET(stats));

    /* ── Status message ──────────────────────────────── */
    page->status_label = GTK_LABEL(gtk_label_new(""));
    gtk_label_set_xalign(page->status_label, 0.0f);
    gtk_widget_add_css_class(GTK_WIDGET(page->status_label), "muted");
    gtk_box_append(page->box, GTK_WIDGET(page->status_label));

    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(page->box, GTK_WIDGET(spacer));

    g_signal_connect(page->refresh_btn, "clicked", G_CALLBACK(on_refresh), page);

    page_profile_refresh(page);
    return page;
}
