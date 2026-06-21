#ifndef INSTALLER_H
#define INSTALLER_H

#include <glib.h>

typedef enum {
    INSTALL_TYPE_DEB,
    INSTALL_TYPE_APPIMAGE,
    INSTALL_TYPE_TARBALL,
    INSTALL_TYPE_RPM,
    INSTALL_TYPE_ZIP,
    INSTALL_TYPE_UNKNOWN
} InstallType;

typedef struct {
    double progress;        /* 0.0 – 1.0 */
    char  *status;          /* human-readable status */
    int    done;            /* 1 when finished */
    int    success;         /* 1 on success */
    char  *install_path;    /* set on success */
    char  *error;           /* set on failure */
} InstallProgress;

typedef struct {
    char *url;
    char *filename;
    char *app_name;
    char *full_name;
    char *version;
    char *repo_url;
    char *token;            /* optional GitHub token */
    long  expected_size;   /* bytes from asset metadata, 0 = unknown */
    InstallProgress *progress;
} InstallTask;

InstallType installer_detect_type(const char *filename);
const char *installer_type_name(InstallType type);

InstallProgress *install_progress_new(void);
void install_progress_free(InstallProgress *p);

InstallTask *install_task_new(void);
void install_task_free(InstallTask *task);

/* Runs in a GLib thread — call g_thread_new(NULL, installer_run, task) */
gpointer installer_run(gpointer data);

/* Uninstall: removes install_path entry. Returns 1 on success. */
int installer_uninstall(const char *install_path, InstallType type);

#endif
