#include "home.h"
#include "../../services/github.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    AppWindow *win;
    GtkEntry *search_entry;
    GtkListBox *results_box;
} HomePageData;

static void on_search_clicked(GtkButton *button, gpointer user_data) {
    HomePageData *data = (HomePageData *)user_data;
    const char *query = gtk_editable_get_text(GTK_EDITABLE(data->search_entry));
    if (!query || !*query) {
        query = "windows app";
    }

    while (gtk_widget_get_first_child(GTK_WIDGET(data->results_box))) {
        gtk_list_box_remove(data->results_box,
            gtk_widget_get_first_child(GTK_WIDGET(data->results_box)));
    }

    GitHubService *svc = github_service_new(data->win->current_token);
    RepositoryList *repos = github_search_repositories(svc, query, 1);

    if (repos) {
        for (int i = 0; i < repos->count && i < 10; i++) {
            GitHubRepository *repo = repos->items[i];

            GtkBox *item_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
            gtk_widget_set_margin_top(GTK_WIDGET(item_box), 8);
            gtk_widget_set_margin_bottom(GTK_WIDGET(item_box), 8);
            gtk_widget_set_margin_start(GTK_WIDGET(item_box), 8);
            gtk_widget_set_margin_end(GTK_WIDGET(item_box), 8);

            char title[256];
            snprintf(title, sizeof(title), "<b>%s</b>", repo->full_name);
            GtkLabel *name_label = GTK_LABEL(gtk_label_new(NULL));
            gtk_label_set_markup(name_label, title);
            gtk_widget_set_halign(GTK_WIDGET(name_label), GTK_ALIGN_START);
            gtk_box_append(item_box, GTK_WIDGET(name_label));

            char desc[512];
            snprintf(desc, sizeof(desc), "<small>%s</small>", repo->description ?: "No description");
            GtkLabel *desc_label = GTK_LABEL(gtk_label_new(NULL));
            gtk_label_set_markup(desc_label, desc);
            gtk_widget_set_halign(GTK_WIDGET(desc_label), GTK_ALIGN_START);
            gtk_box_append(item_box, GTK_WIDGET(desc_label));

            char stats[256];
            snprintf(stats, sizeof(stats), "⭐ %d | Language: %s", repo->stars, repo->language);
            GtkLabel *stats_label = GTK_LABEL(gtk_label_new(stats));
            gtk_widget_set_halign(GTK_WIDGET(stats_label), GTK_ALIGN_START);
            gtk_box_append(item_box, GTK_WIDGET(stats_label));

            gtk_list_box_append(data->results_box, GTK_WIDGET(item_box));
        }
        repository_list_free(repos);
    }

    github_service_free(svc);
}

GtkWidget* page_home_create(AppWindow *win) {
    GtkBox *page = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

    HomePageData *data = malloc(sizeof(HomePageData));
    data->win = win;

    GtkBox *header = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
    gtk_widget_set_margin_top(GTK_WIDGET(header), 16);
    gtk_widget_set_margin_bottom(GTK_WIDGET(header), 16);
    gtk_widget_set_margin_start(GTK_WIDGET(header), 16);
    gtk_widget_set_margin_end(GTK_WIDGET(header), 16);
    gtk_box_set_homogeneous(header, FALSE);

    data->search_entry = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_placeholder_text(data->search_entry, "Search GitHub apps...");
    gtk_widget_set_hexpand(GTK_WIDGET(data->search_entry), TRUE);
    gtk_box_append(header, GTK_WIDGET(data->search_entry));

    GtkButton *search_button = GTK_BUTTON(gtk_button_new_with_label("Search"));
    gtk_box_append(header, GTK_WIDGET(search_button));

    g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_clicked), data);

    gtk_box_append(page, GTK_WIDGET(header));

    data->results_box = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_selection_mode(data->results_box, GTK_SELECTION_NONE);

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(data->results_box));
    gtk_widget_set_hexpand(GTK_WIDGET(scroll), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    gtk_box_append(page, GTK_WIDGET(scroll));

    g_object_set_data_full(G_OBJECT(page), "home-data", data, free);

    on_search_clicked(NULL, data);

    return GTK_WIDGET(page);
}

void page_home_refresh(GtkWidget *page, AppWindow *win) {
}
