#include "window.h"
#include "sidebar.h"
#include "pages/home.h"
#include "pages/library.h"
#include "pages/settings.h"
#include "pages/profile.h"
#include <stdlib.h>

AppWindow* app_window_new(GtkApplication *app) {
    AppWindow *win = malloc(sizeof(AppWindow));

    win->window = GTK_APPLICATION_WINDOW(gtk_application_window_new(app));
    gtk_window_set_title(GTK_WINDOW(win->window), "GitHub App Store");
    gtk_window_set_default_size(GTK_WINDOW(win->window), 1400, 900);

    win->settings = settings_load();
    win->current_token = win->settings->github_token ? strdup(win->settings->github_token) : NULL;

    win->main_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_window_set_child(GTK_WINDOW(win->window), GTK_WIDGET(win->main_box));

    win->sidebar = sidebar_create(win);
    gtk_box_append(win->main_box, GTK_WIDGET(win->sidebar));

    GtkBox *content_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_box_set_homogeneous(content_box, FALSE);
    gtk_box_append(win->main_box, GTK_WIDGET(content_box));
    gtk_widget_set_hexpand(GTK_WIDGET(content_box), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(content_box), TRUE);

    win->page_stack = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(win->page_stack, GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(win->page_stack, 200);

    win->pages[PAGE_HOME] = page_home_create(win);
    gtk_stack_add_named(win->page_stack, win->pages[PAGE_HOME], "home");

    win->pages[PAGE_LIBRARY] = page_library_create(win);
    gtk_stack_add_named(win->page_stack, win->pages[PAGE_LIBRARY], "library");

    win->pages[PAGE_SETTINGS] = page_settings_create(win);
    gtk_stack_add_named(win->page_stack, win->pages[PAGE_SETTINGS], "settings");

    win->pages[PAGE_PROFILE] = page_profile_create(win);
    gtk_stack_add_named(win->page_stack, win->pages[PAGE_PROFILE], "profile");

    gtk_box_append(content_box, GTK_WIDGET(win->page_stack));
    gtk_widget_set_hexpand(GTK_WIDGET(win->page_stack), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(win->page_stack), TRUE);

    return win;
}

void app_window_free(AppWindow *win) {
    if (!win) return;
    settings_free(win->settings);
    free(win->current_token);
    free(win);
}

void app_window_show_page(AppWindow *win, PageType page) {
    if (page >= PAGE_COUNT) return;

    const char *names[] = {"home", "library", "settings", "profile"};
    gtk_stack_set_visible_child_name(win->page_stack, names[page]);

    if (page == PAGE_LIBRARY) {
        page_library_refresh(win->pages[PAGE_LIBRARY], win);
    } else if (page == PAGE_PROFILE) {
        page_profile_refresh(win->pages[PAGE_PROFILE], win);
    }
}
