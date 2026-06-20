#include "library.h"
#include <stdlib.h>

PageLibrary *page_library_new(void) {
    PageLibrary *page = malloc(sizeof(PageLibrary));

    page->box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));
    gtk_widget_set_margin_start(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_end(GTK_WIDGET(page->box), 20);
    gtk_widget_set_margin_top(GTK_WIDGET(page->box), 20);

    GtkLabel *title = GTK_LABEL(gtk_label_new("Library"));
    gtk_widget_add_css_class(GTK_WIDGET(title), "title");
    gtk_box_append(page->box, GTK_WIDGET(title));

    GtkLabel *placeholder = GTK_LABEL(gtk_label_new("No installed apps yet"));
    gtk_widget_set_margin_top(GTK_WIDGET(placeholder), 40);
    gtk_box_append(page->box, GTK_WIDGET(placeholder));

    return page;
}
