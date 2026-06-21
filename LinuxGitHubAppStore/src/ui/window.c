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
    "window { background-color: #eef0f8; }"

    /* ── sidebar ───────────────────────────── */
    "box.sidebar {"
    "  background-color: #1a1c35;"
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
    ".section-sep { margin: 8px 0; opacity: 0.10; }"

    /* ── hero banner ─────────────────────────── */
    ".hero-banner {"
    "  background: linear-gradient(135deg, #6366f1 0%, #8b5cf6 100%);"
    "  border-radius: 16px; padding: 22px 24px 20px 24px; margin-bottom: 4px;"
    "}"
    ".hero-title { font-size: 22px; font-weight: 800; color: white; letter-spacing: -0.01em; }"
    ".hero-sub { font-size: 13px; color: rgba(255,255,255,0.72); }"

    /* ── sidebar section labels ──────────────── */
    ".sidebar-section {"
    "  font-size: 9.5px; font-weight: 700; letter-spacing: 0.12em;"
    "  color: rgba(255,255,255,0.25); padding: 14px 18px 4px 18px;"
    "}"

    /* ── filter tab bar ──────────────────────── */
    ".filter-bar {"
    "  background-color: white; border-radius: 12px;"
    "  border: 1px solid rgba(0,0,0,0.07); padding: 4px;"
    "}"
    ".filter-tab {"
    "  border-radius: 8px; border: none; background: transparent;"
    "  color: rgba(0,0,0,0.55); font-size: 12px; font-weight: 500;"
    "  padding: 6px 14px; min-height: 0;"
    "}"
    ".filter-tab:hover { background-color: rgba(99,102,241,0.08); color: #4338ca; }"
    ".filter-tab-active { background-color: #6366f1; color: white; font-weight: 600; }"
    ".filter-tab-active:hover { background-color: #4f46e5; color: white; }"

    /* ── avatar / initial circles ────────────── */
    ".app-avatar {"
    "  border-radius: 100px; min-width: 44px; min-height: 44px;"
    "  font-size: 18px; font-weight: 700; color: white;"
    "}"
    ".avatar-blue   { background-color: #3b82f6; }"
    ".avatar-green  { background-color: #10b981; }"
    ".avatar-purple { background-color: #8b5cf6; }"
    ".avatar-red    { background-color: #ef4444; }"
    ".avatar-orange { background-color: #f59e0b; }"
    ".avatar-pink   { background-color: #ec4899; }"
    ".avatar-teal   { background-color: #14b8a6; }"

    /* ── detail page banner ──────────────────── */
    ".detail-banner { border-radius: 16px; padding: 18px 20px; margin-bottom: 4px; }"
    ".detail-banner-init {"
    "  font-size: 30px; font-weight: 800; color: white; opacity: 0.90;"
    "  min-width: 56px; min-height: 56px; border-radius: 100px;"
    "  background-color: rgba(255,255,255,0.20);"
    "}"
    ".detail-repo-name { font-size: 17px; font-weight: 700; color: white; }"
    ".detail-repo-meta { font-size: 12px; color: rgba(255,255,255,0.72); }"

    /* ── empty state ─────────────────────────── */
    ".empty-icon { font-size: 52px; opacity: 0.28; }"
    ".empty-text { font-size: 17px; font-weight: 600; opacity: 0.38; }"
    ".empty-hint { font-size: 13px; opacity: 0.30; }"

    /* ── library summary card ────────────────── */
    ".summary-card {"
    "  background: linear-gradient(135deg, rgba(99,102,241,0.08), rgba(139,92,246,0.08));"
    "  border-radius: 12px; border: 1px solid rgba(99,102,241,0.15); padding: 14px 18px;"
    "}"
    ".summary-count { font-size: 26px; font-weight: 800; color: #6366f1; }"
    ".summary-label { font-size: 11px; opacity: 0.48; letter-spacing: 0.03em; }"

    /* ── shadows ─────────────────────────────── */
    "listbox { box-shadow: 0 1px 3px rgba(0,0,0,0.05), 0 4px 16px rgba(0,0,0,0.04); }"
    ".stat-box { box-shadow: 0 1px 3px rgba(0,0,0,0.04), 0 4px 12px rgba(0,0,0,0.04); }"
    "listbox > row { transition: background-color 100ms ease; }"

    /* ── uptime / sysinfo extras ─────────────── */
    ".uptime-val { font-size: 16px; font-weight: 600; }"
    ".info-mono  { font-family: monospace; font-size: 11px; opacity: 0.60; }"

    /* ── profile banner ─────────────────────── */
    ".profile-banner {"
    "  background: linear-gradient(135deg, #4f46e5 0%, #7c3aed 100%);"
    "  border-radius: 16px; padding: 28px 24px 24px 24px; margin-bottom: 4px;"
    "}"
    ".profile-avatar-lg {"
    "  min-width: 72px; min-height: 72px; border-radius: 100px;"
    "  font-size: 30px; font-weight: 800; color: white;"
    "  background-color: rgba(255,255,255,0.22);"
    "  margin-bottom: 12px;"
    "}"
    ".profile-login { font-size: 20px; font-weight: 800; color: white; }"
    ".profile-name  { font-size: 13px; color: rgba(255,255,255,0.75); }"
    ".profile-refresh-btn {"
    "  background-color: rgba(255,255,255,0.15); border: 1px solid rgba(255,255,255,0.25);"
    "  border-radius: 9px; color: white; font-size: 12px; font-weight: 600;"
    "  padding: 6px 14px;"
    "}"
    ".profile-refresh-btn:hover {"
    "  background-color: rgba(255,255,255,0.25);"
    "}"

    /* ── settings cards ─────────────────────── */
    ".settings-card {"
    "  background-color: white; border-radius: 14px;"
    "  border: 1px solid rgba(0,0,0,0.07);"
    "  padding: 18px 20px; margin-bottom: 4px;"
    "  box-shadow: 0 1px 3px rgba(0,0,0,0.04), 0 4px 12px rgba(0,0,0,0.03);"
    "}"
    ".settings-section-title {"
    "  font-size: 13px; font-weight: 700; letter-spacing: 0.01em;"
    "  margin-bottom: 8px;"
    "}"
    ".info-box {"
    "  background-color: rgba(99,102,241,0.06);"
    "  border-radius: 10px; border-left: 3px solid #6366f1;"
    "  padding: 10px 14px; margin-bottom: 4px;"
    "  font-size: 12px; opacity: 0.80;"
    "}"
    "button.neutral-action {"
    "  background-color: #f3f4f6; color: #374151;"
    "  border: 1px solid rgba(0,0,0,0.10); border-radius: 9px;"
    "  font-weight: 600; padding: 7px 16px;"
    "}"
    "button.neutral-action:hover {"
    "  background-color: #e5e7eb;"
    "}"

    /* ── version comparison ─────────────────── */
    ".version-old   { font-size: 12px; opacity: 0.50; text-decoration: line-through; }"
    ".version-arrow { font-size: 13px; color: #6366f1; font-weight: 700; }"
    ".version-new   { font-size: 12px; font-weight: 700; color: #15803d; }"

    /* ── stat icon ──────────────────────────── */
    ".stat-icon { font-size: 20px; opacity: 0.70; margin-bottom: 2px; }"

    /* ── notice box ─────────────────────────── */
    ".notice-box {"
    "  background-color: #fef9c3; border-radius: 10px;"
    "  border: 1px solid rgba(234,179,8,0.25);"
    "  padding: 10px 14px; font-size: 12px; color: #854d0e;"
    "}";

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
