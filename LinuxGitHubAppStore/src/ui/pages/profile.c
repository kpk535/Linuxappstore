#include "profile.h"
#include <stdlib.h>

GtkWidget* page_profile_create(AppWindow *win) {
    GtkBox *page = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

    GtkBox *header = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8));
    gtk_widget_set_margin_top(GTK_WIDGET(header), 20);
    gtk_widget_set_margin_bottom(GTK_WIDGET(header), 20);
    gtk_widget_set_margin_start(GTK_WIDGET(header), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(header), 20);

    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<span size='20000'><b>Profile</b></span>");
    gtk_widget_set_halign(GTK_WIDGET(title), GTK_ALIGN_START);
    gtk_box_append(header, GTK_WIDGET(title));

    GtkLabel *subtitle = GTK_LABEL(gtk_label_new("Your GitHub account and system information"));
    gtk_widget_set_halign(GTK_WIDGET(subtitle), GTK_ALIGN_START);
    gtk_box_append(header, GTK_WIDGET(subtitle));

    gtk_box_append(page, GTK_WIDGET(header));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_widget_set_hexpand(GTK_WIDGET(scroll), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(content));

    GtkBox *status_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8));
    gtk_widget_set_margin_top(GTK_WIDGET(status_box), 16);
    gtk_widget_set_margin_bottom(GTK_WIDGET(status_box), 16);
    gtk_widget_set_margin_start(GTK_WIDGET(status_box), 16);
    gtk_widget_set_margin_end(GTK_WIDGET(status_box), 16);

    if (!win->current_token || !*win->current_token) {
        GtkLabel *no_token = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(no_token, "<span foreground='#f97316'>⚠ No GitHub account connected</span>");
        gtk_widget_set_halign(GTK_WIDGET(no_token), GTK_ALIGN_START);
        gtk_box_append(status_box, GTK_WIDGET(no_token));

        GtkLabel *help = GTK_LABEL(gtk_label_new("Add your Personal Access Token in Settings to connect your GitHub account"));
        gtk_widget_set_halign(GTK_WIDGET(help), GTK_ALIGN_START);
        gtk_label_set_wrap(help, TRUE);
        gtk_box_append(status_box, GTK_WIDGET(help));
    } else {
        GtkLabel *connected = GTK_LABEL(gtk_label_new(NULL));
        gtk_label_set_markup(connected, "<span foreground='#16a34a'>✓ GitHub account connected</span>");
        gtk_widget_set_halign(GTK_WIDGET(connected), GTK_ALIGN_START);
        gtk_box_append(status_box, GTK_WIDGET(connected));
    }

    gtk_box_append(content, GTK_WIDGET(status_box));

    gtk_box_append(page, GTK_WIDGET(scroll));

    return GTK_WIDGET(page);
}

void page_profile_refresh(GtkWidget *page, AppWindow *win) {
}
