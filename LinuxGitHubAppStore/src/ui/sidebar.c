#include "sidebar.h"
#include <stdlib.h>

#define MAX_NAV 8

typedef struct {
    GtkStack  *pages;
    GtkButton *buttons[MAX_NAV];
    int        count;
} NavGroup;

typedef struct {
    NavGroup   *group;
    const char *page;
} NavData;

static void on_nav_clicked(GtkButton *button, gpointer data) {
    NavData *d = (NavData *)data;

    for (int i = 0; i < d->group->count; i++)
        gtk_widget_remove_css_class(GTK_WIDGET(d->group->buttons[i]), "nav-active");

    gtk_widget_add_css_class(GTK_WIDGET(button), "nav-active");
    gtk_stack_set_visible_child_name(d->group->pages, d->page);
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
    gtk_widget_set_margin_start(GTK_WIDGET(sep), 14);
    gtk_widget_set_margin_end(GTK_WIDGET(sep), 14);
    gtk_widget_set_margin_bottom(GTK_WIDGET(sep), 8);
    gtk_widget_set_opacity(GTK_WIDGET(sep), 0.15);
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
    static const int n_items = (int)(sizeof(items)/sizeof(items[0]));

    NavGroup *group = calloc(1, sizeof(NavGroup));
    group->pages = pages;
    group->count = n_items;

    for (int i = 0; i < n_items; i++) {
        GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(items[i].label));
        gtk_widget_add_css_class(GTK_WIDGET(btn), "nav-button");
        gtk_widget_set_size_request(GTK_WIDGET(btn), -1, 44);
        gtk_label_set_xalign(GTK_LABEL(gtk_button_get_child(btn)), 0.0f);

        group->buttons[i] = btn;

        NavData *nd = malloc(sizeof(NavData));
        nd->group = group;
        nd->page  = items[i].page;

        g_signal_connect(btn, "clicked", G_CALLBACK(on_nav_clicked), nd);
        gtk_box_append(sidebar, GTK_WIDGET(btn));
    }

    /* Default: highlight Search */
    gtk_widget_add_css_class(GTK_WIDGET(group->buttons[0]), "nav-active");

    /* Spacer */
    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(sidebar, GTK_WIDGET(spacer));

    GtkLabel *ver = GTK_LABEL(gtk_label_new("v1.1.0  •  Open Source"));
    gtk_widget_add_css_class(GTK_WIDGET(ver), "sidebar-version");
    gtk_widget_set_margin_bottom(GTK_WIDGET(ver), 12);
    gtk_box_append(sidebar, GTK_WIDGET(ver));
}
