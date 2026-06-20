#include "sidebar.h"
#include <stdlib.h>

static void on_nav_clicked(GtkButton *button, gpointer page_name) {
    GtkStack *pages = g_object_get_data(G_OBJECT(button), "pages");
    gtk_stack_set_visible_child_name(pages, (const char *)page_name);
}

void app_sidebar_init(GtkBox *sidebar, GtkStack *pages) {
    GtkBox *nav_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 2));
    gtk_widget_set_margin_top(GTK_WIDGET(nav_box), 10);
    gtk_box_append(sidebar, GTK_WIDGET(nav_box));

    typedef struct { const char *label; const char *page; } NavItem;
    static const NavItem items[] = {
        { "🔍  Search",   "home"     },
        { "📦  Library",  "library"  },
        { "⬆   Updates",  "updates"  },
        { "👤  Profile",  "profile"  },
        { "⚙   Settings", "settings" },
    };

    for (size_t i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(items[i].label));
        gtk_widget_add_css_class(GTK_WIDGET(btn), "nav-button");
        gtk_widget_set_size_request(GTK_WIDGET(btn), -1, 44);
        g_object_set_data(G_OBJECT(btn), "pages", pages);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_nav_clicked),
                         (gpointer)items[i].page);
        gtk_box_append(nav_box, GTK_WIDGET(btn));
    }

    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(sidebar, GTK_WIDGET(spacer));

    GtkLabel *version = GTK_LABEL(gtk_label_new("Linux GitHub App Store v1.0"));
    gtk_widget_set_margin_bottom(GTK_WIDGET(version), 8);
    gtk_box_append(sidebar, GTK_WIDGET(version));
}
