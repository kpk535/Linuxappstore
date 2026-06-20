#include "window.h"
#include "sidebar.h"
#include "pages/home.h"
#include "pages/library.h"
#include "pages/settings.h"
#include "pages/profile.h"
#include "pages/updates.h"
#include "pages/sysinfo.h"
#include <stdlib.h>

/* ── embedded CSS ─────────────────────────────────────────── */
static const char *APP_CSS =
    /* Dark sidebar */
    "box.sidebar {"
    "  background-color: #1a1b2e;"
    "}"
    "box.sidebar label {"
    "  color: rgba(255,255,255,0.55);"
    "}"
    ".sidebar-brand {"
    "  font-size: 15px;"
    "  font-weight: bold;"
    "  color: rgba(255,255,255,0.95);"
    "  padding: 18px 16px 4px 16px;"
    "}"
    ".sidebar-version {"
    "  font-size: 11px;"
    "  color: rgba(255,255,255,0.35);"
    "  padding: 0 16px 14px 16px;"
    "}"
    /* Nav buttons */
    ".nav-button {"
    "  background: transparent;"
    "  border: none;"
    "  border-radius: 8px;"
    "  color: rgba(255,255,255,0.72);"
    "  padding: 9px 14px;"
    "  margin: 1px 8px;"
    "  font-size: 13px;"
    "}"
    ".nav-button:hover {"
    "  background-color: rgba(255,255,255,0.1);"
    "  color: white;"
    "}"
    ".nav-button:active {"
    "  background-color: rgba(255,255,255,0.15);"
    "}"
    /* Page headings */
    ".page-title {"
    "  font-size: 22px;"
    "  font-weight: bold;"
    "  margin-bottom: 4px;"
    "}"
    ".page-subtitle {"
    "  font-size: 13px;"
    "  opacity: 0.6;"
    "  margin-bottom: 8px;"
    "}"
    /* Section headings inside pages */
    ".section-header {"
    "  font-size: 13px;"
    "  font-weight: bold;"
    "  letter-spacing: 0.08em;"
    "  opacity: 0.55;"
    "  margin-top: 8px;"
    "  margin-bottom: 4px;"
    "}"
    /* List rows */
    "listbox {"
    "  border-radius: 10px;"
    "  border: 1px solid rgba(0,0,0,0.08);"
    "}"
    "listbox row {"
    "  border-radius: 8px;"
    "  padding: 2px 2px;"
    "}"
    "listbox row:hover {"
    "  background-color: rgba(99,102,241,0.07);"
    "}"
    "listbox row:selected {"
    "  background-color: rgba(99,102,241,0.14);"
    "}"
    /* Status colours */
    ".success { color: #16a34a; font-weight: 600; }"
    ".error   { color: #dc2626; font-weight: 600; }"
    ".warning { color: #d97706; font-weight: 600; }"
    ".muted   { opacity: 0.55; }"
    /* Chip / category buttons */
    ".chip {"
    "  border-radius: 20px;"
    "  padding: 5px 14px;"
    "  font-size: 13px;"
    "  background-color: rgba(99,102,241,0.12);"
    "  color: #4f46e5;"
    "  border: 1px solid rgba(99,102,241,0.25);"
    "}"
    ".chip:hover {"
    "  background-color: rgba(99,102,241,0.2);"
    "}"
    /* Stats box on profile/sysinfo */
    ".stat-box {"
    "  border-radius: 10px;"
    "  border: 1px solid rgba(0,0,0,0.09);"
    "  padding: 12px 16px;"
    "  margin: 3px;"
    "}"
    ".stat-value {"
    "  font-size: 20px;"
    "  font-weight: bold;"
    "}"
    ".stat-label {"
    "  font-size: 12px;"
    "  opacity: 0.55;"
    "}"
    /* Token entry */
    ".token-entry { font-family: monospace; }"
    /* Separator */
    ".section-sep { margin: 8px 0; }";

AppWindow *app_window_new(GtkApplication *app) {
    /* Apply CSS globally */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, APP_CSS);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    AppWindow *win = malloc(sizeof(AppWindow));

    win->window = GTK_APPLICATION_WINDOW(gtk_application_window_new(app));
    gtk_window_set_title(GTK_WINDOW(win->window), "Linux GitHub App Store");
    gtk_window_set_default_size(GTK_WINDOW(win->window), 1050, 700);

    win->main_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_window_set_child(GTK_WINDOW(win->window), GTK_WIDGET(win->main_box));

    win->sidebar = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_size_request(GTK_WIDGET(win->sidebar), 225, -1);
    gtk_widget_add_css_class(GTK_WIDGET(win->sidebar), "sidebar");
    gtk_box_append(win->main_box, GTK_WIDGET(win->sidebar));

    win->pages = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(win->pages, GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(win->pages, 150);
    gtk_box_append(win->main_box, GTK_WIDGET(win->pages));
    gtk_widget_set_hexpand(GTK_WIDGET(win->pages), TRUE);

    app_sidebar_init(win->sidebar, win->pages);

    PageHome     *home     = page_home_new();
    PageLibrary  *library  = page_library_new();
    PageUpdates  *updates  = page_updates_new();
    PageProfile  *profile  = page_profile_new();
    PageSysinfo  *sysinfo  = page_sysinfo_new();
    PageSettings *settings = page_settings_new();

    gtk_stack_add_named(win->pages, GTK_WIDGET(home->box),     "home");
    gtk_stack_add_named(win->pages, GTK_WIDGET(library->box),  "library");
    gtk_stack_add_named(win->pages, GTK_WIDGET(updates->box),  "updates");
    gtk_stack_add_named(win->pages, GTK_WIDGET(profile->box),  "profile");
    gtk_stack_add_named(win->pages, GTK_WIDGET(sysinfo->box),  "sysinfo");
    gtk_stack_add_named(win->pages, GTK_WIDGET(settings->box), "settings");

    gtk_stack_set_visible_child_name(win->pages, "home");
    return win;
}

void app_window_show(AppWindow *win) {
    gtk_window_present(GTK_WINDOW(win->window));
}

void app_window_switch_page(AppWindow *win, const char *page_name) {
    if (gtk_stack_get_child_by_name(win->pages, page_name))
        gtk_stack_set_visible_child_name(win->pages, page_name);
}
