#include <gtk/gtk.h>
#include "ui/window.h"

static void on_activate(GtkApplication *app, gpointer user_data __attribute__((unused))) {
    AppWindow *win = app_window_new(app);
    app_window_show_page(win, PAGE_HOME);
    gtk_window_present(GTK_WINDOW(win->window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("com.github.linux-appstore", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
