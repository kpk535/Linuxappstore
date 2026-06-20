#include "sidebar.h"
#include <stdlib.h>

static void on_nav_clicked(GtkButton *button, gpointer page_name) {
    GtkStack *pages = g_object_get_data(G_OBJECT(button), "pages");
    gtk_stack_set_visible_child_name(pages, (const char *)page_name);
}

void app_sidebar_init(GtkBox *sidebar, GtkStack *pages) {
    /* Brand */
    GtkLabel *brand = GTK_LABEL(gtk_label_new("GitHub App Store"));
    gtk_widget_add_css_class(GTK_WIDGET(brand), "sidebar-brand");
    gtk_label_set_xalign(brand, 0.0f);
    gtk_box_append(sidebar, GTK_WIDGET(brand));

    GtkLabel *sub = GTK_LABEL(gtk_label_new("for Linux"));
    gtk_widget_add_css_class(GTK_WIDGET(sub), "sidebar-version");
    gtk_label_set_xalign(sub, 0.0f);
    gtk_box_append(sidebar, GTK_WIDGET(sub));

    GtkSeparator *sep = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_widget_set_margin_start(GTK_WIDGET(sep), 12);
    gtk_widget_set_margin_end(GTK_WIDGET(sep), 12);
    gtk_widget_set_margin_bottom(GTK_WIDGET(sep), 6);
    gtk_box_append(sidebar, GTK_WIDGET(sep));

    /* Nav items */
    typedef struct { const char *label; const char *page; } NavItem;
    static const NavItem items[] = {
        { "🔍  Search",      "home"     },
        { "📦  Library",     "library"  },
        { "⬆   Updates",     "updates"  },
        { "👤  Profile",     "profile"  },
        { "💻  System Info", "sysinfo"  },
        { "⚙   Settings",   "settings" },
    };

    for (size_t i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(items[i].label));
        gtk_widget_add_css_class(GTK_WIDGET(btn), "nav-button");
        gtk_widget_set_size_request(GTK_WIDGET(btn), -1, 42);
        gtk_label_set_xalign(
            GTK_LABEL(gtk_button_get_child(btn)), 0.0f);
        g_object_set_data(G_OBJECT(btn), "pages", pages);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_nav_clicked),
                         (gpointer)items[i].page);
        gtk_box_append(sidebar, GTK_WIDGET(btn));
    }

    /* Spacer */
    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(sidebar, GTK_WIDGET(spacer));

    GtkLabel *ver = GTK_LABEL(gtk_label_new("v1.1.0  •  Open Source"));
    gtk_widget_add_css_class(GTK_WIDGET(ver), "sidebar-version");
    gtk_widget_set_margin_bottom(GTK_WIDGET(ver), 10);
    gtk_box_append(sidebar, GTK_WIDGET(ver));
}
