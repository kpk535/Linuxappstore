#include "sidebar.h"
#include "window.h"
#include <stdlib.h>

typedef struct {
    AppWindow *win;
    PageType page;
} NavButtonData;

static void on_nav_button_clicked(GtkButton *button __attribute__((unused)), gpointer user_data) {
    NavButtonData *data = (NavButtonData *)user_data;
    app_window_show_page(data->win, data->page);
}

static GtkButton* create_nav_button(const char *icon, const char *label, PageType page, AppWindow *win) {
    GtkButton *button = GTK_BUTTON(gtk_button_new());
    GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8));
    gtk_button_set_child(button, GTK_WIDGET(hbox));

    GtkLabel *icon_label = GTK_LABEL(gtk_label_new(icon));
    gtk_label_set_markup(icon_label, icon);
    gtk_box_append(hbox, GTK_WIDGET(icon_label));

    GtkLabel *text_label = GTK_LABEL(gtk_label_new(label));
    gtk_box_append(hbox, GTK_WIDGET(text_label));

    NavButtonData *data = malloc(sizeof(NavButtonData));
    data->win = win;
    data->page = page;

    g_signal_connect(button, "clicked", G_CALLBACK(on_nav_button_clicked), data);

    return button;
}

GtkBox* sidebar_create(AppWindow *win) {
    GtkBox *sidebar = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_size_request(GTK_WIDGET(sidebar), 240, -1);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css,
        "box { background-color: #f8f9fa; border-right: 1px solid #e0e0e0; }"
        "button { padding: 12px 16px; border: none; background: transparent; "
        "  text-align: left; font-size: 14px; }"
        "button:hover { background-color: #ececec; }"
        "button:active { background-color: #e0e0e0; }");
    gtk_widget_add_css_class(GTK_WIDGET(sidebar), "sidebar");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkBox *logo_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
    gtk_widget_set_margin_top(GTK_WIDGET(logo_box), 20);
    gtk_widget_set_margin_bottom(GTK_WIDGET(logo_box), 20);
    gtk_widget_set_margin_start(GTK_WIDGET(logo_box), 16);

    GtkLabel *title = GTK_LABEL(gtk_label_new("GitHub Store"));
    gtk_label_set_markup(title, "<b>GitHub Store</b>");
    gtk_widget_set_halign(GTK_WIDGET(title), GTK_ALIGN_START);
    gtk_box_append(logo_box, GTK_WIDGET(title));

    GtkLabel *subtitle = GTK_LABEL(gtk_label_new("Open-source app catalog"));
    gtk_label_set_markup(subtitle, "<small>Open-source app catalog</small>");
    gtk_widget_set_halign(GTK_WIDGET(subtitle), GTK_ALIGN_START);
    gtk_box_append(logo_box, GTK_WIDGET(subtitle));

    gtk_box_append(sidebar, GTK_WIDGET(logo_box));

    GtkSeparator *sep1 = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(sidebar, GTK_WIDGET(sep1));

    gtk_box_append(sidebar, GTK_WIDGET(create_nav_button("⌂", "Home", PAGE_HOME, win)));
    gtk_box_append(sidebar, GTK_WIDGET(create_nav_button("📂", "Library", PAGE_LIBRARY, win)));

    GtkSeparator *sep2 = GTK_SEPARATOR(gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_box_append(sidebar, GTK_WIDGET(sep2));

    GtkLabel *account_label = GTK_LABEL(gtk_label_new("ACCOUNT"));
    gtk_label_set_markup(account_label, "<small>ACCOUNT</small>");
    gtk_widget_set_margin_top(GTK_WIDGET(account_label), 12);
    gtk_widget_set_margin_bottom(GTK_WIDGET(account_label), 8);
    gtk_widget_set_margin_start(GTK_WIDGET(account_label), 16);
    gtk_widget_set_halign(GTK_WIDGET(account_label), GTK_ALIGN_START);
    gtk_box_append(sidebar, GTK_WIDGET(account_label));

    gtk_box_append(sidebar, GTK_WIDGET(create_nav_button("👤", "Profile", PAGE_PROFILE, win)));
    gtk_box_append(sidebar, GTK_WIDGET(create_nav_button("⚙", "Settings", PAGE_SETTINGS, win)));

    gtk_box_set_spacing(sidebar, 4);

    return sidebar;
}

void sidebar_set_nav_callback(GtkBox *sidebar __attribute__((unused)),
                             void (*callback)(PageType) __attribute__((unused))) {
}
