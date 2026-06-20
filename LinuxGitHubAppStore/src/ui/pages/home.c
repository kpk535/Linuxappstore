#include "home.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include <stdlib.h>
#include <glib.h>

typedef struct {
    char *name;
    char *description;
    char *url;
    int stars;
} ResultItem;

static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    PageHome *page = (PageHome *)user_data;
    const char *query = gtk_editable_get_text(GTK_EDITABLE(entry));

    while (gtk_widget_get_first_child(GTK_WIDGET(page->results_list))) {
        gtk_list_box_remove(page->results_list, gtk_widget_get_first_child(GTK_WIDGET(page->results_list)));
    }

    if (strlen(query) < 2) return;

    AppSettings *settings = settings_load();
    GitHubService *service = github_service_new(settings->github_token);

    int count = 0;
    GitHubRepository **repos = github_search_repositories(service, query, &count);

    for (int i = 0; i < count; i++) {
        GtkBox *item_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        gtk_widget_set_margin_start(GTK_WIDGET(item_box), 10);
        gtk_widget_set_margin_end(GTK_WIDGET(item_box), 10);
        gtk_widget_set_margin_top(GTK_WIDGET(item_box), 10);
        gtk_widget_set_margin_bottom(GTK_WIDGET(item_box), 10);

        char title[256];
        snprintf(title, sizeof(title), "<b>%s</b> - ⭐ %d stars", repos[i]->name, repos[i]->stars);
        GtkLabel *title_label = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(title_label, title);
        gtk_label_set_wrap(title_label, TRUE);
        gtk_box_append(item_box, GTK_WIDGET(title_label));

        GtkLabel *desc_label = GTK_LABEL(gtk_label_new(repos[i]->description));
        gtk_label_set_wrap(desc_label, TRUE);
        gtk_widget_add_css_class(GTK_WIDGET(desc_label), "description");
        gtk_box_append(item_box, GTK_WIDGET(desc_label));

        GtkListBoxRow *row = GTK_LIST_BOX_ROW(gtk_list_box_row_new());
        gtk_list_box_row_set_child(row, GTK_WIDGET(item_box));
        gtk_list_box_append(page->results_list, GTK_WIDGET(row));

        repository_free(repos[i]);
    }

    free(repos);
    github_service_free(service);
    settings_free(settings);
}

PageHome *page_home_new(void) {
    PageHome *page = malloc(sizeof(PageHome));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 20);

    GtkLabel *title = GTK_LABEL(gtk_label_new("Search Apps"));
    gtk_widget_add_css_class(GTK_WIDGET(title), "title");
    gtk_box_append(page->box, GTK_WIDGET(title));

    page->search_entry = GTK_SEARCH_ENTRY(gtk_search_entry_new());
    gtk_widget_set_size_request(GTK_WIDGET(page->search_entry), -1, 40);
    gtk_box_append(page->box, GTK_WIDGET(page->search_entry));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_append(page->box, GTK_WIDGET(scroll));
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    page->results_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(page->results_list));

    g_signal_connect(page->search_entry, "search-changed", G_CALLBACK(on_search_changed), page);

    return page;
}
