#include "installer.h"
#include "package_db.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <curl/curl.h>

/* ── type detection ────────────────────────────────────────────── */

InstallType installer_detect_type(const char *filename) {
    if (!filename) return INSTALL_TYPE_UNKNOWN;
    if (g_str_has_suffix(filename, ".AppImage") ||
        g_str_has_suffix(filename, ".appimage"))  return INSTALL_TYPE_APPIMAGE;
    if (g_str_has_suffix(filename, ".deb"))       return INSTALL_TYPE_DEB;
    if (g_str_has_suffix(filename, ".rpm"))       return INSTALL_TYPE_RPM;
    if (g_str_has_suffix(filename, ".zip"))       return INSTALL_TYPE_ZIP;
    if (g_str_has_suffix(filename, ".tar.gz")  ||
        g_str_has_suffix(filename, ".tar.xz")  ||
        g_str_has_suffix(filename, ".tar.bz2") ||
        g_str_has_suffix(filename, ".tgz"))       return INSTALL_TYPE_TARBALL;
    return INSTALL_TYPE_UNKNOWN;
}

const char *installer_type_name(InstallType type) {
    switch (type) {
        case INSTALL_TYPE_DEB:      return "deb";
        case INSTALL_TYPE_APPIMAGE: return "appimage";
        case INSTALL_TYPE_TARBALL:  return "tarball";
        case INSTALL_TYPE_RPM:      return "rpm";
        case INSTALL_TYPE_ZIP:      return "zip";
        default:                    return "unknown";
    }
}

/* ── progress / task lifecycle ─────────────────────────────────── */

InstallProgress *install_progress_new(void) { return calloc(1, sizeof(InstallProgress)); }

void install_progress_free(InstallProgress *p) {
    if (!p) return;
    free(p->status);
    free(p->install_path);
    free(p->error);
    free(p);
}

InstallTask *install_task_new(void) { return calloc(1, sizeof(InstallTask)); }

void install_task_free(InstallTask *task) {
    if (!task) return;
    free(task->url);      free(task->filename); free(task->app_name);
    free(task->full_name); free(task->version); free(task->repo_url);
    free(task->token);
    free(task);
}

/* ── disk space check ──────────────────────────────────────────── */

static int check_disk_space(const char *dir, long required_bytes,
                             InstallProgress *progress) {
    struct statvfs vfs;
    if (statvfs(dir, &vfs) != 0) return 1; /* can't check — allow */
    long free_bytes = (long)vfs.f_bavail * (long)vfs.f_frsize;
    long needed     = required_bytes + 64L * 1024 * 1024; /* +64 MB buffer */
    if (free_bytes < needed) {
        char msg[192];
        snprintf(msg, sizeof(msg),
                 "Not enough disk space in %s: need %ld MB, have %ld MB free.",
                 dir, needed / (1024*1024), free_bytes / (1024*1024));
        free(progress->error);
        progress->error = strdup(msg);
        return 0;
    }
    return 1;
}

/* ── file size check after download ───────────────────────────── */

static long file_size(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) ? (long)st.st_size : -1;
}

/* ── curl write callback ───────────────────────────────────────── */

typedef struct { FILE *fp; InstallProgress *progress; } WriteCtx;

static size_t write_file_cb(void *ptr, size_t size, size_t nmemb, void *userp) {
    WriteCtx *ctx = (WriteCtx *)userp;
    return fwrite(ptr, size, nmemb, ctx->fp);
}

static int xferinfo_cb(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                       curl_off_t u1, curl_off_t u2) {
    (void)u1; (void)u2;
    InstallProgress *p = (InstallProgress *)clientp;
    if (dltotal > 0)
        p->progress = (double)dlnow / (double)dltotal * 0.80;
    return 0;
}

/* ── download ──────────────────────────────────────────────────── */

static int download_file(const char *url, const char *dest,
                         const char *token, InstallProgress *progress,
                         long *out_http_code) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        free(progress->error);
        progress->error = strdup("Failed to initialize HTTP client");
        return 0;
    }

    FILE *fp = fopen(dest, "wb");
    if (!fp) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Cannot open temp file for writing: %s", strerror(errno));
        free(progress->error); progress->error = strdup(msg);
        curl_easy_cleanup(curl);
        return 0;
    }

    WriteCtx ctx = { .fp = fp, .progress = progress };
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/octet-stream");
    if (token) {
        char auth[320];
        snprintf(auth, sizeof(auth), "Authorization: token %s", token);
        headers = curl_slist_append(headers, auth);
    }

    curl_easy_setopt(curl, CURLOPT_URL,              url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,        "LinuxGitHubAppStore/1.1");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,   1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS,        10L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,       headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,    write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,        &ctx);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA,     progress);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,   15L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT,  1024L);   /* 1 KB/s minimum */
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME,   30L);     /* abort if slow for 30s */

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    fclose(fp);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (out_http_code) *out_http_code = http_code;

    if (res != CURLE_OK) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Download failed: %s", curl_easy_strerror(res));
        free(progress->error); progress->error = strdup(msg);
        unlink(dest);
        return 0;
    }
    if (http_code == 401) {
        free(progress->error);
        progress->error = strdup("Download failed: GitHub token is invalid (401)");
        unlink(dest);
        return 0;
    }
    if (http_code == 403) {
        free(progress->error);
        progress->error = strdup("Download failed: access forbidden (403) — check token permissions");
        unlink(dest);
        return 0;
    }
    if (http_code == 404) {
        free(progress->error);
        progress->error = strdup("Download failed: asset not found (404)");
        unlink(dest);
        return 0;
    }
    if (http_code >= 500) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Download failed: GitHub server error (%ld)", http_code);
        free(progress->error); progress->error = strdup(msg);
        unlink(dest);
        return 0;
    }
    return 1;
}

/* ── apps directory ────────────────────────────────────────────── */

static const char *get_apps_dir(void) {
    const char *home = getenv("HOME") ?: "/root";
    static char dir[512];
    snprintf(dir, sizeof(dir), "%s/Applications", home);
    mkdir(dir, 0755);
    return dir;
}

/* ── desktop file creation ─────────────────────────────────────── */

static void create_desktop_file(const char *app_name, const char *exec_path,
                                const char *full_name) {
    const char *home = getenv("HOME") ?: "/root";
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/.local/share/applications", home);
    mkdir(dir, 0755);

    /* sanitize app name for filename */
    char safe_name[128];
    strncpy(safe_name, app_name, sizeof(safe_name) - 1);
    safe_name[sizeof(safe_name)-1] = '\0';
    for (char *p = safe_name; *p; p++)
        if (*p == '/' || *p == ' ' || *p == '\\') *p = '-';

    char path[640];
    snprintf(path, sizeof(path), "%s/%s.desktop", dir, safe_name);

    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f,
        "[Desktop Entry]\n"
        "Version=1.0\n"
        "Type=Application\n"
        "Name=%s\n"
        "Exec=%s\n"
        "Icon=application-x-executable\n"
        "Categories=Utility;\n"
        "Comment=Installed from GitHub — %s\n"
        "Terminal=false\n"
        "StartupNotify=true\n",
        app_name, exec_path, full_name ?: "");
    fclose(f);
    chmod(path, 0644);
}

/* ── per-type installation ─────────────────────────────────────── */

static char *install_appimage(const char *src, const char *app_name,
                              const char *full_name) {
    char dest[512];
    snprintf(dest, sizeof(dest), "%s/%s.AppImage", get_apps_dir(), app_name);
    if (rename(src, dest) != 0) {
        /* rename across filesystems: fall back to copy */
        char cmd[1200];
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s'", src, dest);
        if (system(cmd) != 0) return NULL;
    }
    if (chmod(dest, 0755) != 0) return NULL;
    create_desktop_file(app_name, dest, full_name);
    return strdup(dest);
}

static char *install_deb(const char *src) {
    /* Try pkexec first for GUI password prompt, fall back to sudo */
    char cmd[768];
    snprintf(cmd, sizeof(cmd),
             "pkexec dpkg -i '%s' 2>/dev/null || sudo dpkg -i '%s' 2>&1"
             " || pkexec apt-get install -y '%s' 2>/dev/null"
             " || sudo apt-get install -y '%s' 2>&1",
             src, src, src, src);
    return (system(cmd) == 0) ? strdup(src) : NULL;
}

static char *install_tarball(const char *src, const char *app_name) {
    char dest_dir[512];
    snprintf(dest_dir, sizeof(dest_dir), "%s/%s", get_apps_dir(), app_name);
    mkdir(dest_dir, 0755);
    char cmd[1200];
    snprintf(cmd, sizeof(cmd),
             "tar xf '%s' -C '%s' --strip-components=1 2>&1", src, dest_dir);
    return (system(cmd) == 0) ? strdup(dest_dir) : NULL;
}

static char *install_zip(const char *src, const char *app_name) {
    char dest_dir[512];
    snprintf(dest_dir, sizeof(dest_dir), "%s/%s", get_apps_dir(), app_name);
    mkdir(dest_dir, 0755);
    char cmd[1200];
    snprintf(cmd, sizeof(cmd),
             "unzip -q -o '%s' -d '%s' 2>&1", src, dest_dir);
    return (system(cmd) == 0) ? strdup(dest_dir) : NULL;
}

static char *install_rpm(const char *src) {
    char cmd[768];
    snprintf(cmd, sizeof(cmd),
             "pkexec rpm -i '%s' 2>/dev/null || sudo rpm -i '%s' 2>&1",
             src, src);
    return (system(cmd) == 0) ? strdup(src) : NULL;
}

/* ── main thread entry ─────────────────────────────────────────── */

gpointer installer_run(gpointer data) {
    InstallTask     *task     = (InstallTask *)data;
    InstallProgress *progress = task->progress;

    const char *tmpdir = getenv("TMPDIR") ?: "/tmp";
    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s/%s", tmpdir, task->filename);

    /* ── 1. Disk space check ── */
    if (task->expected_size > 0) {
        free(progress->status);
        progress->status = strdup("Checking disk space…");

        if (!check_disk_space(tmpdir, task->expected_size, progress)) {
            progress->done = 1;
            return NULL;
        }
        if (!check_disk_space(get_apps_dir(), task->expected_size, progress)) {
            progress->done = 1;
            return NULL;
        }
    }

    /* ── 2. Download ── */
    free(progress->status);
    progress->status = strdup("Downloading…");

    long http_code = 0;
    int downloaded = download_file(task->url, tmp_path, task->token, progress, &http_code);

    if (!downloaded) {
        progress->done = 1;
        return NULL;
    }

    /* ── 3. Validate downloaded file ── */
    long actual_size = file_size(tmp_path);
    if (actual_size < 512) {
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "Download appears corrupted: file is only %ld bytes.", actual_size);
        free(progress->error); progress->error = strdup(msg);
        free(progress->status); progress->status = strdup("Download validation failed");
        unlink(tmp_path);
        progress->done = 1;
        return NULL;
    }

    if (task->expected_size > 0) {
        long diff = actual_size - task->expected_size;
        if (diff < 0) diff = -diff;
        /* Allow up to 5% size difference (compression differences, etc.) */
        if (diff > task->expected_size / 20 + 1024) {
            char msg[192];
            snprintf(msg, sizeof(msg),
                     "Download size mismatch: expected %ld bytes, got %ld bytes. "
                     "File may be corrupted.",
                     task->expected_size, actual_size);
            free(progress->error); progress->error = strdup(msg);
            free(progress->status); progress->status = strdup("File validation failed");
            unlink(tmp_path);
            progress->done = 1;
            return NULL;
        }
    }

    /* ── 4. Install ── */
    progress->progress = 0.85;
    free(progress->status);
    progress->status = strdup("Installing…");

    InstallType type = installer_detect_type(task->filename);
    char *install_path = NULL;

    switch (type) {
        case INSTALL_TYPE_APPIMAGE:
            install_path = install_appimage(tmp_path, task->app_name, task->full_name);
            break;
        case INSTALL_TYPE_DEB:
            install_path = install_deb(tmp_path);
            break;
        case INSTALL_TYPE_TARBALL:
            install_path = install_tarball(tmp_path, task->app_name);
            break;
        case INSTALL_TYPE_ZIP:
            install_path = install_zip(tmp_path, task->app_name);
            break;
        case INSTALL_TYPE_RPM:
            install_path = install_rpm(tmp_path);
            break;
        default:
            /* Unknown — copy to Applications with original name */
            install_path = install_appimage(tmp_path, task->app_name, task->full_name);
            break;
    }

    unlink(tmp_path);

    /* ── 5. Result ── */
    if (install_path) {
        progress->progress    = 1.0;
        progress->install_path = install_path;
        progress->success      = 1;
        free(progress->status);
        progress->status = strdup("Installed successfully ✓");
    } else {
        free(progress->status);
        progress->status = strdup("Installation failed");
        if (!progress->error) {
            const char *type_name = installer_type_name(type);
            char msg[256];
            if (type == INSTALL_TYPE_DEB || type == INSTALL_TYPE_RPM) {
                snprintf(msg, sizeof(msg),
                         "Failed to install %s package. "
                         "Ensure pkexec or sudo is available and you have admin rights.",
                         type_name);
            } else {
                snprintf(msg, sizeof(msg),
                         "Failed to install %s. "
                         "Check that required tools (tar, unzip) are installed.",
                         type_name);
            }
            progress->error = strdup(msg);
        }
    }

    progress->done = 1;
    return NULL;
}

/* ── uninstall ──────────────────────────────────────────────────── */

int installer_uninstall(const char *install_path, InstallType type) {
    if (!install_path || strlen(install_path) < 4) return 0;
    char cmd[768];

    switch (type) {
        case INSTALL_TYPE_DEB:
        case INSTALL_TYPE_RPM:
            /* System packages — skip path-based removal */
            return 0;
        case INSTALL_TYPE_APPIMAGE:
            snprintf(cmd, sizeof(cmd), "rm -f '%s'", install_path);
            break;
        default:
            /* Tarball / zip directories */
            if (strstr(install_path, "..")) return 0; /* safety: reject path traversal */
            snprintf(cmd, sizeof(cmd), "rm -rf '%s'", install_path);
            break;
    }

    /* Also try to remove the .desktop file */
    const char *home = getenv("HOME") ?: "/root";
    const char *base = strrchr(install_path, '/');
    if (base) {
        base++;
        char desktop[640];
        /* strip .AppImage suffix */
        char app_name[256];
        strncpy(app_name, base, sizeof(app_name) - 1);
        app_name[sizeof(app_name)-1] = '\0';
        char *dot = strrchr(app_name, '.');
        if (dot) *dot = '\0';
        snprintf(desktop, sizeof(desktop),
                 "%s/.local/share/applications/%s.desktop", home, app_name);
        unlink(desktop);
    }

    return system(cmd) == 0;
}
