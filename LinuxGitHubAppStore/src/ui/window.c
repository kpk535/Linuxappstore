#include "window.h"
#include "sidebar.h"
#include "pages/home.h"
#include "pages/library.h"
#include "pages/settings.h"
#include "pages/profile.h"
#include "pages/updates.h"
#include <stdlib.h>

AppWindow *app_window_new(GtkApplication *app) {
    AppWindow *win = malloc(sizeof(AppWindow));

    win->window = GTK_APPLICATION_WINDOW(gtk_application_window_new(app));
    gtk_window_set_title(GTK_WINDOW(win->window), "Linux GitHub App Store");
    gtk_window_set_default_size(GTK_WINDOW(win->window), 1000, 650);

    win->main_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_window_set_child(GTK_WINDOW(win->window), GTK_WIDGET(win->main_box));

    win->sidebar = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_size_request(GTK_WIDGET(win->sidebar), 220, -1);
    gtk_widget_add_css_class(GTK_WIDGET(win->sidebar), "sidebar");
    gtk_box_append(win->main_box, GTK_WIDGET(win->sidebar));

    win->pages = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(win->pages, GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_box_append(win->main_box, GTK_WIDGET(win->pages));
    gtk_widget_set_hexpand(GTK_WIDGET(win->pages), TRUE);

    app_sidebar_init(win->sidebar, win->pages);

    PageHome *home = page_home_new();
    gtk_stack_add_named(win->pages, GTK_WIDGET(home->box), "home");

    PageLibrary *library = page_library_new();
    gtk_stack_add_named(win->pages, GTK_WIDGET(library->box), "library");

    PageUpdates *updates = page_updates_new();
    gtk_stack_add_named(win->pages, GTK_WIDGET(updates->box), "updates");

    PageProfile *profile = page_profile_new();
    gtk_stack_add_named(win->pages, GTK_WIDGET(profile->box), "profile");

    PageSettings *settings = page_settings_new();
    gtk_stack_add_named(win->pages, GTK_WIDGET(settings->box), "settings");

    gtk_stack_set_visible_child_name(win->pages, "home");
    return win;
}

void app_window_show(AppWindow *win) {
    gtk_window_present(GTK_WINDOW(win->window));
}

void app_window_switch_page(AppWindow *win, const char *page_name) {
    if (gtk_stack_get_child_by_name(win->pages, page_name))
        gtk_stack_set_visible_child_name(win->pages, page_name);
}
