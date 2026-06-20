#include "sidebar.h"
#include <stdlib.h>

static void on_nav_clicked(GtkButton *button, gpointer user_data) {
    const char *page = (const char *)user_data;
    GtkStack *pages = g_object_get_data(G_OBJECT(button), "pages");
    gtk_stack_set_visible_child_name(pages, page);
}

void app_sidebar_init(GtkBox *sidebar, GtkStack *pages) {
    GtkBox *nav_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_add_css_class(GTK_WIDGET(nav_box), "nav-box");
    gtk_box_append(sidebar, GTK_WIDGET(nav_box));

    const char *nav_items[] = {"Home", "Library", "Profile", "Settings"};
    const char *nav_pages[] = {"home", "library", "profile", "settings"};

    for (int i = 0; i < 4; i++) {
        GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(nav_items[i]));
        gtk_widget_add_css_class(GTK_WIDGET(btn), "nav-button");
        g_object_set_data(G_OBJECT(btn), "pages", pages);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_nav_clicked), (gpointer)nav_pages[i]);
        gtk_box_append(nav_box, GTK_WIDGET(btn));
    }

    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(sidebar, GTK_WIDGET(spacer));
}
