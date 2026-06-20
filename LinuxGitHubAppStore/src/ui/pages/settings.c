#include "settings.h"
#include "../../services/github.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    AppWindow *win;
    GtkEntry *token_entry;
    GtkLabel *status_label;
    GtkButton *test_button;
} SettingsPageData;

static void on_test_token_clicked(GtkButton *button, gpointer user_data) {
    SettingsPageData *data = (SettingsPageData *)user_data;
    const char *token = gtk_editable_get_text(GTK_EDITABLE(data->token_entry));

    gtk_button_set_label(data->test_button, "Testing...");
    gtk_widget_set_sensitive(GTK_WIDGET(data->test_button), FALSE);

    GitHubService *svc = github_service_new(token);
    char *profile = github_get_user_profile(svc);

    if (profile && *profile) {
        gtk_label_set_markup(data->status_label, "<span foreground='green'>✓ Token is valid</span>");
        if (data->win->current_token) free(data->win->current_token);
        data->win->current_token = strdup(token);
        data->win->settings->github_token = data->win->current_token;
    } else {
        gtk_label_set_markup(data->status_label, "<span foreground='red'>✗ Token is invalid</span>");
    }

    github_service_free(svc);
    free(profile);

    gtk_button_set_label(data->test_button, "Test Token");
    gtk_widget_set_sensitive(GTK_WIDGET(data->test_button), TRUE);
}

GtkWidget* page_settings_create(AppWindow *win) {
    GtkBox *page = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

    SettingsPageData *data = malloc(sizeof(SettingsPageData));
    data->win = win;

    GtkBox *header = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8));
    gtk_widget_set_margin_top(GTK_WIDGET(header), 20);
    gtk_widget_set_margin_bottom(GTK_WIDGET(header), 20);
    gtk_widget_set_margin_start(GTK_WIDGET(header), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(header), 20);

    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<span size='20000'><b>Settings</b></span>");
    gtk_widget_set_halign(GTK_WIDGET(title), GTK_ALIGN_START);
    gtk_box_append(header, GTK_WIDGET(title));

    GtkLabel *subtitle = GTK_LABEL(gtk_label_new("Configure your app store preferences"));
    gtk_widget_set_halign(GTK_WIDGET(subtitle), GTK_ALIGN_START);
    gtk_box_append(header, GTK_WIDGET(subtitle));

    gtk_box_append(page, GTK_WIDGET(header));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_widget_set_hexpand(GTK_WIDGET(scroll), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(content));

    GtkBox *github_section = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_margin_top(GTK_WIDGET(github_section), 16);
    gtk_widget_set_margin_start(GTK_WIDGET(github_section), 16);
    gtk_widget_set_margin_end(GTK_WIDGET(github_section), 16);

    GtkLabel *github_title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(github_title, "<b>GitHub Settings</b>");
    gtk_widget_set_halign(GTK_WIDGET(github_title), GTK_ALIGN_START);
    gtk_box_append(github_section, GTK_WIDGET(github_title));

    GtkSeparator *sep = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_widget_set_margin_top(GTK_WIDGET(sep), 8);
    gtk_widget_set_margin_bottom(GTK_WIDGET(sep), 8);
    gtk_box_append(github_section, GTK_WIDGET(sep));

    GtkBox *token_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_widget_set_margin_bottom(GTK_WIDGET(token_row), 16);

    GtkBox *token_labels = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
    GtkLabel *token_label = GTK_LABEL(gtk_label_new("Personal Access Token"));
    gtk_widget_set_halign(GTK_WIDGET(token_label), GTK_ALIGN_START);
    gtk_box_append(token_labels, GTK_WIDGET(token_label));

    GtkLabel *token_help = GTK_LABEL(gtk_label_new("Increases API rate limit from 60 to 5,000 requests/hour"));
    gtk_label_set_markup(token_help, "<small>Increases API rate limit from 60 to 5,000 requests/hour</small>");
    gtk_widget_set_halign(GTK_WIDGET(token_help), GTK_ALIGN_START);
    gtk_box_append(token_labels, GTK_WIDGET(token_help));

    gtk_widget_set_hexpand(GTK_WIDGET(token_labels), TRUE);
    gtk_box_append(token_row, GTK_WIDGET(token_labels));

    data->token_entry = GTK_ENTRY(gtk_entry_new());
    gtk_editable_set_text(GTK_EDITABLE(data->token_entry), win->current_token ?: "");
    gtk_entry_set_visibility(data->token_entry, FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(data->token_entry), 240, -1);
    gtk_box_append(token_row, GTK_WIDGET(data->token_entry));

    gtk_box_append(github_section, GTK_WIDGET(token_row));

    GtkBox *test_row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12));
    gtk_widget_set_margin_bottom(GTK_WIDGET(test_row), 16);

    GtkBox *test_labels = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
    data->status_label = GTK_LABEL(gtk_label_new("Test your token to verify it works"));
    gtk_widget_set_halign(GTK_WIDGET(data->status_label), GTK_ALIGN_START);
    gtk_label_set_wrap(data->status_label, TRUE);
    gtk_box_append(test_labels, GTK_WIDGET(data->status_label));

    gtk_widget_set_hexpand(GTK_WIDGET(test_labels), TRUE);
    gtk_box_append(test_row, GTK_WIDGET(test_labels));

    data->test_button = GTK_BUTTON(gtk_button_new_with_label("Test Token"));
    gtk_widget_set_size_request(GTK_WIDGET(data->test_button), 100, -1);
    gtk_box_append(test_row, GTK_WIDGET(data->test_button));

    g_signal_connect(data->test_button, "clicked", G_CALLBACK(on_test_token_clicked), data);

    gtk_box_append(github_section, GTK_WIDGET(test_row));
    gtk_box_append(content, GTK_WIDGET(github_section));

    gtk_box_append(page, GTK_WIDGET(scroll));

    g_object_set_data_full(G_OBJECT(page), "settings-data", data, free);

    return GTK_WIDGET(page);
}

void page_settings_refresh(GtkWidget *page, AppWindow *win) {
}
