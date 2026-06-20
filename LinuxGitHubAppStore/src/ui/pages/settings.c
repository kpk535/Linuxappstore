#include "settings.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include <stdlib.h>
#include <string.h>

static void on_test_token(GtkButton *button, gpointer user_data) {
    PageSettings *page = (PageSettings *)user_data;
    const char *token = gtk_editable_get_text(GTK_EDITABLE(page->token_entry));

    if (!token || strlen(token) == 0) {
        gtk_label_set_text(page->status_label, "Please enter a token");
        return;
    }

    gtk_label_set_text(page->status_label, "Testing token...");

    GitHubService *service = github_service_new(token);
    char *result = github_validate_token(service);

    if (strcmp(result, "true") == 0) {
        gtk_label_set_text(page->status_label, "✓ Token is valid");
        gtk_widget_add_css_class(GTK_WIDGET(page->status_label), "success");
        gtk_widget_remove_css_class(GTK_WIDGET(page->status_label), "error");

        AppSettings *settings = settings_load();
        free(settings->github_token);
        settings->github_token = strdup(token);
        settings_save(settings);
        settings_free(settings);
    } else {
        gtk_label_set_text(page->status_label, "✗ Token is invalid");
        gtk_widget_add_css_class(GTK_WIDGET(page->status_label), "error");
        gtk_widget_remove_css_class(GTK_WIDGET(page->status_label), "success");
    }

    free(result);
    github_service_free(service);
}

PageSettings *page_settings_new(void) {
    PageSettings *page = malloc(sizeof(PageSettings));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 20);

    GtkLabel *title = GTK_LABEL(gtk_label_new("Settings"));
    gtk_widget_add_css_class(GTK_WIDGET(title), "title");
    gtk_box_append(page->box, GTK_WIDGET(title));

    GtkLabel *token_label = GTK_LABEL(gtk_label_new("GitHub Personal Access Token:"));
    gtk_box_append(page->box, GTK_WIDGET(token_label));

    page->token_entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_visibility(page->token_entry, FALSE);
    gtk_entry_set_placeholder_text(page->token_entry, "Enter your GitHub token");
    gtk_widget_set_size_request(GTK_WIDGET(page->token_entry), -1, 40);

    AppSettings *settings = settings_load();
    if (settings->github_token) {
        gtk_editable_set_text(GTK_EDITABLE(page->token_entry), settings->github_token);
    }
    settings_free(settings);

    gtk_box_append(page->box, GTK_WIDGET(page->token_entry));

    page->test_btn = GTK_BUTTON(gtk_button_new_with_label("Test Token"));
    g_signal_connect(page->test_btn, "clicked", G_CALLBACK(on_test_token), page);
    gtk_box_append(page->box, GTK_WIDGET(page->test_btn));

    page->status_label = GTK_LABEL(gtk_label_new(""));
    gtk_box_append(page->box, GTK_WIDGET(page->status_label));

    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(page->box, GTK_WIDGET(spacer));

    return page;
}
