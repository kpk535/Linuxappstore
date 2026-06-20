#include "installer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>

/* ── helpers ───────────────────────────────────────────────── */

InstallType installer_detect_type(const char *filename) {
    if (!filename) return INSTALL_TYPE_UNKNOWN;
    const char *dot = strrchr(filename, '.');
    if (!dot) return INSTALL_TYPE_UNKNOWN;
    if (strcmp(dot, ".deb") == 0)                        return INSTALL_TYPE_DEB;
    if (strcmp(dot, ".rpm") == 0)                        return INSTALL_TYPE_RPM;
    if (strcmp(dot, ".zip") == 0)                        return INSTALL_TYPE_ZIP;
    if (g_str_has_suffix(filename, ".tar.gz") ||
        g_str_has_suffix(filename, ".tar.xz") ||
        g_str_has_suffix(filename, ".tar.bz2") ||
        strcmp(dot, ".tgz") == 0)                        return INSTALL_TYPE_TARBALL;
    if (g_str_has_suffix(filename, ".AppImage") ||
        g_str_has_suffix(filename, ".appimage"))         return INSTALL_TYPE_APPIMAGE;
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

InstallProgress *install_progress_new(void) {
    return calloc(1, sizeof(InstallProgress));
}

void install_progress_free(InstallProgress *p) {
    if (!p) return;
    free(p->status);
    free(p->install_path);
    free(p->error);
    free(p);
}

InstallTask *install_task_new(void) {
    return calloc(1, sizeof(InstallTask));
}

void install_task_free(InstallTask *task) {
    if (!task) return;
    free(task->url);
    free(task->filename);
    free(task->app_name);
    free(task->full_name);
    free(task->version);
    free(task->repo_url);
    free(task->token);
    free(task);
}

/* ── curl download ─────────────────────────────────────────── */

typedef struct {
    FILE          *fp;
    InstallProgress *progress;
} WriteCtx;

static size_t write_file_cb(void *ptr, size_t size, size_t nmemb, void *userp) {
    WriteCtx *ctx = (WriteCtx *)userp;
    return fwrite(ptr, size, nmemb, ctx->fp);
}

static int xferinfo_cb(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                       curl_off_t UNUSED1, curl_off_t UNUSED2) {
    (void)UNUSED1; (void)UNUSED2;
    InstallProgress *p = (InstallProgress *)clientp;
    if (dltotal > 0)
        p->progress = (double)dlnow / (double)dltotal * 0.8; /* 0–80% for download */
    return 0;
}

static int download_file(const char *url, const char *dest, const char *token,
                         InstallProgress *progress) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    FILE *fp = fopen(dest, "wb");
    if (!fp) { curl_easy_cleanup(curl); return 0; }

    WriteCtx ctx = { .fp = fp, .progress = progress };

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/octet-stream");
    if (token) {
        char auth[300];
        snprintf(auth, sizeof(auth), "Authorization: token %s", token);
        headers = curl_slist_append(headers, auth);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, progress);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

/* ── per-type installation ─────────────────────────────────── */

static char *get_apps_dir(void) {
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    static char dir[256];
    snprintf(dir, sizeof(dir), "%s/Applications", home);
    mkdir(dir, 0755);
    return dir;
}

static char *install_appimage(const char *src, const char *app_name) {
    char *apps_dir = get_apps_dir();
    char dest[512];
    snprintf(dest, sizeof(dest), "%s/%s.AppImage", apps_dir, app_name);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cp '%s' '%s' && chmod +x '%s'", src, dest, dest);
    if (system(cmd) != 0) return NULL;
    return strdup(dest);
}

static char *install_deb(const char *src) {
    char cmd[512];
    /* Try pkexec first; fall back to sudo */
    snprintf(cmd, sizeof(cmd),
             "pkexec apt-get install -y '%s' 2>/dev/null || sudo apt-get install -y '%s' 2>&1",
             src, src);
    int ret = system(cmd);
    return (ret == 0) ? strdup(src) : NULL;
}

static char *install_tarball(const char *src, const char *app_name) {
    char *apps_dir = get_apps_dir();
    char dest_dir[512];
    snprintf(dest_dir, sizeof(dest_dir), "%s/%s", apps_dir, app_name);
    mkdir(dest_dir, 0755);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "tar xf '%s' -C '%s' --strip-components=1 2>&1", src, dest_dir);
    if (system(cmd) != 0) return NULL;
    return strdup(dest_dir);
}

static char *install_zip(const char *src, const char *app_name) {
    char *apps_dir = get_apps_dir();
    char dest_dir[512];
    snprintf(dest_dir, sizeof(dest_dir), "%s/%s", apps_dir, app_name);
    mkdir(dest_dir, 0755);
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "unzip -q -o '%s' -d '%s' 2>&1", src, dest_dir);
    if (system(cmd) != 0) return NULL;
    return strdup(dest_dir);
}

static char *install_rpm(const char *src) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "pkexec rpm -i '%s' 2>/dev/null || sudo rpm -i '%s' 2>&1", src, src);
    int ret = system(cmd);
    return (ret == 0) ? strdup(src) : NULL;
}

/* ── main thread entry ─────────────────────────────────────── */

gpointer installer_run(gpointer data) {
    InstallTask     *task     = (InstallTask *)data;
    InstallProgress *progress = task->progress;

    /* temp download path */
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = "/tmp";
    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s/%s", tmpdir, task->filename);

    free(progress->status);
    progress->status = strdup("Downloading...");

    if (!download_file(task->url, tmp_path, task->token, progress)) {
        free(progress->status);
        progress->status = strdup("Download failed");
        free(progress->error);
        progress->error = strdup("curl download failed");
        progress->done = 1;
        return NULL;
    }

    progress->progress = 0.85;
    free(progress->status);
    progress->status = strdup("Installing...");

    InstallType type = installer_detect_type(task->filename);
    char *install_path = NULL;

    switch (type) {
        case INSTALL_TYPE_APPIMAGE:
            install_path = install_appimage(tmp_path, task->app_name);
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
            /* Unknown — just copy to Applications */
            install_path = install_appimage(tmp_path, task->app_name);
            break;
    }

    unlink(tmp_path); /* remove temp file */

    if (install_path) {
        progress->progress    = 1.0;
        progress->install_path = install_path;
        progress->success     = 1;
        free(progress->status);
        progress->status = strdup("Installed successfully");
    } else {
        free(progress->status);
        progress->status = strdup("Installation failed");
        free(progress->error);
        progress->error = strdup("Installer returned non-zero");
    }

    progress->done = 1;
    return NULL;
}

int installer_uninstall(const char *install_path, InstallType type) {
    if (!install_path) return 0;
    char cmd[512];

    switch (type) {
        case INSTALL_TYPE_DEB:
            /* Can't easily uninstall by path; skip */
            return 0;
        case INSTALL_TYPE_APPIMAGE:
            snprintf(cmd, sizeof(cmd), "rm -f '%s'", install_path);
            break;
        default:
            snprintf(cmd, sizeof(cmd), "rm -rf '%s'", install_path);
            break;
    }

    return system(cmd) == 0;
}
