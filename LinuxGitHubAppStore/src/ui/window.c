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

    /* ── window background ─────────────────── */
    "window { background-color: #f0f2f7; }"

    /* ── sidebar ───────────────────────────── */
    "box.sidebar {"
    "  background-color: #16172b;"
    "  border-right: 1px solid rgba(0,0,0,0.25);"
    "}"
    "box.sidebar > label {"
    "  color: rgba(255,255,255,0.45);"
    "}"
    ".sidebar-brand {"
    "  font-size: 14px;"
    "  font-weight: 800;"
    "  color: white;"
    "  padding: 22px 18px 1px 18px;"
    "  letter-spacing: 0.01em;"
    "}"
    ".sidebar-version {"
    "  font-size: 10px;"
    "  color: rgba(255,255,255,0.22);"
    "  padding: 0 18px 18px 18px;"
    "}"

    /* ── nav buttons ───────────────────────── */
    ".nav-button {"
    "  background: transparent;"
    "  border: none;"
    "  border-radius: 10px;"
    "  color: rgba(255,255,255,0.58);"
    "  padding: 10px 14px;"
    "  margin: 2px 10px;"
    "  font-size: 13px;"
    "}"
    ".nav-button:hover {"
    "  background-color: rgba(255,255,255,0.07);"
    "  color: rgba(255,255,255,0.88);"
    "}"
    ".nav-button:active {"
    "  background-color: rgba(255,255,255,0.11);"
    "}"
    ".nav-active {"
    "  background-color: rgba(99,102,241,0.28);"
    "  color: white;"
    "}"
    ".nav-active label {"
    "  color: white;"
    "  font-weight: 600;"
    "}"
    ".nav-active:hover {"
    "  background-color: rgba(99,102,241,0.35);"
    "}"

    /* ── page titles ───────────────────────── */
    ".page-title {"
    "  font-size: 24px;"
    "  font-weight: 800;"
    "  letter-spacing: -0.02em;"
    "  margin-bottom: 2px;"
    "}"
    ".page-subtitle {"
    "  font-size: 13px;"
    "  opacity: 0.48;"
    "  margin-bottom: 10px;"
    "}"
    ".section-header {"
    "  font-size: 10.5px;"
    "  font-weight: 700;"
    "  letter-spacing: 0.10em;"
    "  opacity: 0.38;"
    "  margin-top: 14px;"
    "  margin-bottom: 6px;"
    "}"

    /* ── list box (white card) ──────────────── */
    "listbox {"
    "  background-color: white;"
    "  border-radius: 14px;"
    "  border: 1px solid rgba(0,0,0,0.07);"
    "}"
    "listbox > row {"
    "  padding: 0;"
    "}"
    "listbox > row:first-child {"
    "  border-radius: 14px 14px 0 0;"
    "}"
    "listbox > row:last-child {"
    "  border-radius: 0 0 14px 14px;"
    "}"
    "listbox > row:only-child {"
    "  border-radius: 14px;"
    "}"
    "listbox > row:hover {"
    "  background-color: rgba(99,102,241,0.05);"
    "}"
    "listbox > row:selected {"
    "  background-color: rgba(99,102,241,0.10);"
    "}"

    /* ── search / entry ────────────────────── */
    "searchentry {"
    "  border-radius: 14px;"
    "  padding: 4px 8px;"
    "  font-size: 14px;"
    "  background-color: white;"
    "  border: 1.5px solid rgba(0,0,0,0.09);"
    "  min-height: 44px;"
    "}"
    "searchentry:focus-within {"
    "  border-color: #6366f1;"
    "}"
    "entry, .token-entry {"
    "  border-radius: 12px;"
    "  min-height: 42px;"
    "  background-color: white;"
    "  border: 1.5px solid rgba(0,0,0,0.09);"
    "  font-family: monospace;"
    "}"
    "entry:focus {"
    "  border-color: #6366f1;"
    "}"

    /* ── status ────────────────────────────── */
    ".success { color: #15803d; font-weight: 600; }"
    ".error   { color: #dc2626; font-weight: 600; }"
    ".warning { color: #d97706; font-weight: 600; }"
    ".muted   { opacity: 0.48; }"

    /* ── category chips ─────────────────────── */
    ".chip {"
    "  border-radius: 100px;"
    "  padding: 5px 14px;"
    "  font-size: 12px;"
    "  font-weight: 500;"
    "  background-color: rgba(99,102,241,0.10);"
    "  color: #4338ca;"
    "  border: 1px solid rgba(99,102,241,0.20);"
    "}"
    ".chip:hover {"
    "  background-color: rgba(99,102,241,0.18);"
    "  color: #3730a3;"
    "}"

    /* ── stat boxes ─────────────────────────── */
    ".stat-box {"
    "  background-color: white;"
    "  border-radius: 14px;"
    "  border: 1px solid rgba(0,0,0,0.07);"
    "  padding: 16px 12px;"
    "  margin: 4px;"
    "}"
    ".stat-value {"
    "  font-size: 21px;"
    "  font-weight: 700;"
    "}"
    ".stat-label {"
    "  font-size: 11px;"
    "  opacity: 0.42;"
    "  letter-spacing: 0.03em;"
    "}"

    /* ── action buttons ─────────────────────── */
    "button.suggested-action {"
    "  background-color: #6366f1;"
    "  color: white;"
    "  border: none;"
    "  border-radius: 9px;"
    "  font-weight: 600;"
    "  padding: 7px 16px;"
    "}"
    "button.suggested-action:hover {"
    "  background-color: #4f46e5;"
    "}"
    "button.destructive-action {"
    "  background-color: #fee2e2;"
    "  color: #b91c1c;"
    "  border: 1px solid rgba(220,38,38,0.15);"
    "  border-radius: 9px;"
    "  font-weight: 600;"
    "}"
    "button.destructive-action:hover {"
    "  background-color: #fecaca;"
    "}"

    /* ── progress bar ───────────────────────── */
    "progressbar trough {"
    "  border-radius: 100px;"
    "  background-color: rgba(0,0,0,0.08);"
    "  min-height: 8px;"
    "}"
    "progressbar progress {"
    "  border-radius: 100px;"
    "  background-color: #6366f1;"
    "}"
    "progressbar.ram progress { background-color: #06b6d4; }"
    "progressbar.disk progress { background-color: #8b5cf6; }"
    "progressbar.update progress { background-color: #f59e0b; }"

    /* ── type badges ────────────────────────── */
    ".badge-appimage {"
    "  background-color: #dbeafe; color: #1d4ed8;"
    "  border-radius: 6px; padding: 2px 8px;"
    "  font-size: 11px; font-weight: 700;"
    "}"
    ".badge-deb {"
    "  background-color: #dcfce7; color: #15803d;"
    "  border-radius: 6px; padding: 2px 8px;"
    "  font-size: 11px; font-weight: 700;"
    "}"
    ".badge-rpm {"
    "  background-color: #fce7f3; color: #9d174d;"
    "  border-radius: 6px; padding: 2px 8px;"
    "  font-size: 11px; font-weight: 700;"
    "}"
    ".badge-zip {"
    "  background-color: #fef3c7; color: #92400e;"
    "  border-radius: 6px; padding: 2px 8px;"
    "  font-size: 11px; font-weight: 700;"
    "}"
    ".badge-tar {"
    "  background-color: #ede9fe; color: #6d28d9;"
    "  border-radius: 6px; padding: 2px 8px;"
    "  font-size: 11px; font-weight: 700;"
    "}"
    ".badge-update {"
    "  background-color: #fef9c3; color: #854d0e;"
    "  border-radius: 6px; padding: 2px 8px;"
    "  font-size: 11px; font-weight: 700;"
    "}"
    ".badge-ok {"
    "  background-color: #dcfce7; color: #166534;"
    "  border-radius: 6px; padding: 2px 8px;"
    "  font-size: 11px; font-weight: 700;"
    "}"
    ".badge-lang {"
    "  background-color: #f3f4f6; color: #374151;"
    "  border-radius: 6px; padding: 2px 8px;"
    "  font-size: 11px;"
    "}"

    /* ── section separator ──────────────────── */
    ".section-sep { margin: 8px 0; opacity: 0.10; }";

AppWindow *app_window_new(GtkApplication *app) {
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
    gtk_window_set_default_size(GTK_WINDOW(win->window), 1100, 720);

    win->main_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    gtk_window_set_child(GTK_WINDOW(win->window), GTK_WIDGET(win->main_box));

    win->sidebar = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_size_request(GTK_WIDGET(win->sidebar), 230, -1);
    gtk_widget_add_css_class(GTK_WIDGET(win->sidebar), "sidebar");
    gtk_box_append(win->main_box, GTK_WIDGET(win->sidebar));

    win->pages = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(win->pages, GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(win->pages, 180);
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
