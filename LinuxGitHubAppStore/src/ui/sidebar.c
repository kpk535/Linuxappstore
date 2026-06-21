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
    int         idx;
} NavData;

static void on_nav_clicked(GtkButton *button, gpointer data) {
    (void)button;
    NavData *d = (NavData *)data;

    for (int i = 0; i < d->group->count; i++)
        gtk_widget_remove_css_class(GTK_WIDGET(d->group->buttons[i]), "nav-active");

    gtk_widget_add_css_class(GTK_WIDGET(d->group->buttons[d->idx]), "nav-active");
    gtk_stack_set_visible_child_name(d->group->pages, d->page);
}

static void add_section_label(GtkBox *sidebar, const char *text) {
    GtkLabel *lbl = GTK_LABEL(gtk_label_new(text));
    gtk_widget_add_css_class(GTK_WIDGET(lbl), "sidebar-section");
    gtk_label_set_xalign(lbl, 0.0f);
    gtk_box_append(sidebar, GTK_WIDGET(lbl));
}

void app_sidebar_init(GtkBox *sidebar, GtkStack *pages) {
    /* Brand area */
    GtkBox *brand_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 2));
    gtk_widget_set_margin_top(GTK_WIDGET(brand_box), 4);

    GtkLabel *brand = GTK_LABEL(gtk_label_new("GitHub App Store"));
    gtk_widget_add_css_class(GTK_WIDGET(brand), "sidebar-brand");
    gtk_label_set_xalign(brand, 0.0f);
    gtk_box_append(brand_box, GTK_WIDGET(brand));

    GtkLabel *brand_sub = GTK_LABEL(gtk_label_new("for Linux"));
    gtk_widget_add_css_class(GTK_WIDGET(brand_sub), "sidebar-version");
    gtk_label_set_xalign(brand_sub, 0.0f);
    gtk_box_append(brand_box, GTK_WIDGET(brand_sub));
    gtk_box_append(sidebar, GTK_WIDGET(brand_box));

    GtkSeparator *sep = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_widget_set_margin_start(GTK_WIDGET(sep), 14);
    gtk_widget_set_margin_end(GTK_WIDGET(sep), 14);
    gtk_widget_set_margin_top(GTK_WIDGET(sep), 10);
    gtk_widget_set_margin_bottom(GTK_WIDGET(sep), 4);
    gtk_widget_set_opacity(GTK_WIDGET(sep), 0.15);
    gtk_box_append(sidebar, GTK_WIDGET(sep));

    /* Nav definitions — NULL page means section label */
    typedef struct {
        const char *label;
        const char *page;
        const char *tooltip;
    } NavDef;

    static const NavDef defs[] = {
        { "APPS",          NULL,       NULL },
        { "🔍  Search",    "home",     "Search GitHub repositories for installable apps" },
        { "📦  Library",   "library",  "View and manage your installed applications" },
        { "⬆   Updates",   "updates",  "Check installed apps for newer releases" },
        { "ACCOUNT",       NULL,       NULL },
        { "👤  Profile",   "profile",  "View your GitHub account profile and stats" },
        { "SYSTEM",        NULL,       NULL },
        { "💻  System",    "sysinfo",  "Hardware and operating system information" },
        { "⚙   Settings",  "settings", "Configure GitHub token and app preferences" },
    };
    static const int n_defs = (int)(sizeof(defs) / sizeof(defs[0]));

    NavGroup *group = calloc(1, sizeof(NavGroup));
    group->pages = pages;
    group->count = 0;

    for (int i = 0; i < n_defs; i++) {
        if (!defs[i].page) {
            add_section_label(sidebar, defs[i].label);
            continue;
        }

        GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(defs[i].label));
        gtk_widget_add_css_class(GTK_WIDGET(btn), "nav-button");
        gtk_widget_set_size_request(GTK_WIDGET(btn), -1, 42);
        gtk_label_set_xalign(GTK_LABEL(gtk_button_get_child(btn)), 0.0f);

        if (defs[i].tooltip)
            gtk_widget_set_tooltip_text(GTK_WIDGET(btn), defs[i].tooltip);

        int idx = group->count;
        group->buttons[idx] = btn;
        group->count++;

        NavData *nd = malloc(sizeof(NavData));
        nd->group = group;
        nd->page  = defs[i].page;
        nd->idx   = idx;

        g_signal_connect(btn, "clicked", G_CALLBACK(on_nav_clicked), nd);
        gtk_box_append(sidebar, GTK_WIDGET(btn));
    }

    /* Default active: Search (index 0) */
    if (group->count > 0)
        gtk_widget_add_css_class(GTK_WIDGET(group->buttons[0]), "nav-active");

    /* Spacer + version at bottom */
    GtkBox *spacer = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_vexpand(GTK_WIDGET(spacer), TRUE);
    gtk_box_append(sidebar, GTK_WIDGET(spacer));

    GtkLabel *ver = GTK_LABEL(gtk_label_new("v1.2.0  •  Open Source"));
    gtk_widget_add_css_class(GTK_WIDGET(ver), "sidebar-version");
    gtk_widget_set_margin_bottom(GTK_WIDGET(ver), 12);
    gtk_box_append(sidebar, GTK_WIDGET(ver));
}
