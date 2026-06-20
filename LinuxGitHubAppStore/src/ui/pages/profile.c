#include "profile.h"
#include "../../services/github.h"
#include "../../services/settings.h"
#include <stdlib.h>
#include <json-c/json.h>
#include <string.h>

PageProfile *page_profile_new(void) {
    PageProfile *page = malloc(sizeof(PageProfile));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 20);

    GtkLabel *title = GTK_LABEL(gtk_label_new("GitHub Profile"));
    gtk_widget_add_css_class(GTK_WIDGET(title), "title");
    gtk_box_append(page->box, GTK_WIDGET(title));

    page->username_label = GTK_LABEL(gtk_label_new("Username: Not signed in"));
    gtk_box_append(page->box, GTK_WIDGET(page->username_label));

    page->name_label = GTK_LABEL(gtk_label_new("Name: -"));
    gtk_box_append(page->box, GTK_WIDGET(page->name_label));

    GtkButton *go_to_settings = GTK_BUTTON(gtk_button_new_with_label("Go to Settings"));
    gtk_box_append(page->box, GTK_WIDGET(go_to_settings));

    AppSettings *settings = settings_load();
    if (settings->github_token) {
        GitHubService *service = github_service_new(settings->github_token);
        char *profile_json = github_get_user_profile(service);

        json_object *parsed = json_tokener_parse(profile_json);
        if (parsed) {
            json_object *login_obj, *name_obj;
            if (json_object_object_get_ex(parsed, "login", &login_obj)) {
                char username[128];
                snprintf(username, sizeof(username), "Username: %s", json_object_get_string(login_obj));
                gtk_label_set_text(page->username_label, username);
            }
            if (json_object_object_get_ex(parsed, "name", &name_obj)) {
                const char *name = json_object_get_string(name_obj);
                if (name && strlen(name) > 0) {
                    char full_name[128];
                    snprintf(full_name, sizeof(full_name), "Name: %s", name);
                    gtk_label_set_text(page->name_label, full_name);
                }
            }
            json_object_put(parsed);
        }

        free(profile_json);
        github_service_free(service);
    }
    settings_free(settings);

    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(page->box, GTK_WIDGET(spacer));

    return page;
}
