#include <gtk/gtk.h>
#include "ui/window.h"

static void activate(GtkApplication *app, gpointer user_data) {
    AppWindow *win = app_window_new(app);
    app_window_show(win);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.github.linuxappstore", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
