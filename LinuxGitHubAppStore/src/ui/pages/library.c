#include "library.h"
#include <stdlib.h>

GtkWidget* page_library_create(AppWindow *win) {
    GtkBox *page = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));

    GtkBox *header = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8));
    gtk_widget_set_margin_top(GTK_WIDGET(header), 16);
    gtk_widget_set_margin_bottom(GTK_WIDGET(header), 16);
    gtk_widget_set_margin_start(GTK_WIDGET(header), 16);
    gtk_widget_set_margin_end(GTK_WIDGET(header), 16);

    GtkLabel *title = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(title, "<b>Library</b>");
    gtk_label_set_markup(title, "<span size='20000'><b>Library</b></span>");
    gtk_widget_set_halign(GTK_WIDGET(title), GTK_ALIGN_START);
    gtk_box_append(header, GTK_WIDGET(title));

    GtkLabel *subtitle = GTK_LABEL(gtk_label_new("Your installed applications"));
    gtk_widget_set_halign(GTK_WIDGET(subtitle), GTK_ALIGN_START);
    gtk_box_append(header, GTK_WIDGET(subtitle));

    gtk_box_append(page, GTK_WIDGET(header));

    GtkListBox *list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_selection_mode(list, GTK_SELECTION_NONE);

    GtkLabel *empty = GTK_LABEL(gtk_label_new("No apps installed yet"));
    gtk_widget_set_margin_top(GTK_WIDGET(empty), 40);
    gtk_list_box_append(list, GTK_WIDGET(empty));

    GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_child(scroll, GTK_WIDGET(list));
    gtk_widget_set_hexpand(GTK_WIDGET(scroll), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(scroll), TRUE);

    gtk_box_append(page, GTK_WIDGET(scroll));

    return GTK_WIDGET(page);
}

void page_library_refresh(GtkWidget *page, AppWindow *win) {
}
