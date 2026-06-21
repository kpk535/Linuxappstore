#include "package_db.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <time.h>

#define DB_DIR  ".config/linux-github-appstore"
#define DB_FILE "packages.json"

static char *get_db_path(void) {
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    static char path[512];
    snprintf(path, sizeof(path), "%s/%s/%s", home, DB_DIR, DB_FILE);
    return path;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (fread(buf, 1, sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    buf[sz] = 0;
    fclose(f);
    return buf;
}

InstalledApp **package_db_load(int *count) {
    *count = 0;
    char *content = read_file(get_db_path());
    if (!content) return NULL;

    json_object *root = json_tokener_parse(content);
    free(content);
    if (!root) return NULL;

    json_object *arr;
    if (!json_object_object_get_ex(root, "installed", &arr)) {
        json_object_put(root);
        return NULL;
    }

    int len = json_object_array_length(arr);
    InstalledApp **apps = malloc(sizeof(InstalledApp *) * (len + 1));

    for (int i = 0; i < len; i++) {
        json_object *obj = json_object_array_get_idx(arr, i);
        InstalledApp *app = installed_app_new();
        json_object *v;

        if (json_object_object_get_ex(obj, "name", &v))           app->name           = strdup(json_object_get_string(v));
        if (json_object_object_get_ex(obj, "full_name", &v))      app->full_name      = strdup(json_object_get_string(v));
        if (json_object_object_get_ex(obj, "version", &v))        app->version        = strdup(json_object_get_string(v));
        if (json_object_object_get_ex(obj, "install_path", &v))   app->install_path   = strdup(json_object_get_string(v));
        if (json_object_object_get_ex(obj, "install_type", &v))   app->install_type   = strdup(json_object_get_string(v));
        if (json_object_object_get_ex(obj, "repo_url", &v))       app->repo_url       = strdup(json_object_get_string(v));
        if (json_object_object_get_ex(obj, "installed_at", &v))   app->installed_at   = strdup(json_object_get_string(v));
        if (json_object_object_get_ex(obj, "latest_version", &v)) app->latest_version = strdup(json_object_get_string(v));
        if (json_object_object_get_ex(obj, "has_update", &v))     app->has_update     = json_object_get_boolean(v);

        apps[i] = app;
    }

    *count = len;
    apps[len] = NULL;
    json_object_put(root);
    return apps;
}

void package_db_save(InstalledApp **apps, int count) {
    const char *home = getenv("HOME");
    if (!home) home = "/root";
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/%s", home, DB_DIR);
    mkdir(dir, 0755);

    json_object *root = json_object_new_object();
    json_object *arr  = json_object_new_array();

    for (int i = 0; i < count; i++) {
        InstalledApp *app = apps[i];
        json_object *obj = json_object_new_object();
        json_object_object_add(obj, "name",           json_object_new_string(app->name           ?: ""));
        json_object_object_add(obj, "full_name",      json_object_new_string(app->full_name      ?: ""));
        json_object_object_add(obj, "version",        json_object_new_string(app->version        ?: ""));
        json_object_object_add(obj, "install_path",   json_object_new_string(app->install_path   ?: ""));
        json_object_object_add(obj, "install_type",   json_object_new_string(app->install_type   ?: ""));
        json_object_object_add(obj, "repo_url",       json_object_new_string(app->repo_url       ?: ""));
        json_object_object_add(obj, "installed_at",   json_object_new_string(app->installed_at   ?: ""));
        json_object_object_add(obj, "latest_version", json_object_new_string(app->latest_version ?: ""));
        json_object_object_add(obj, "has_update",     json_object_new_boolean(app->has_update));
        json_object_array_add(arr, obj);
    }

    json_object_object_add(root, "installed", arr);
    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);

    FILE *f = fopen(get_db_path(), "w");
    if (f) { fprintf(f, "%s\n", json_str); fclose(f); }
    json_object_put(root);
}

void package_db_add(InstalledApp *app) {
    int count = 0;
    InstalledApp **apps = package_db_load(&count);

    /* replace if already exists */
    for (int i = 0; i < count; i++) {
        if (apps[i]->full_name && app->full_name &&
            strcmp(apps[i]->full_name, app->full_name) == 0) {
            installed_app_free(apps[i]);
            apps[i] = app;
            package_db_save(apps, count);
            free(apps);
            return;
        }
    }

    InstalledApp **new_apps = malloc(sizeof(InstalledApp *) * (count + 1));
    for (int i = 0; i < count; i++) new_apps[i] = apps[i];
    new_apps[count] = app;
    package_db_save(new_apps, count + 1);
    free(new_apps);
    free(apps);
}

void package_db_remove(const char *full_name) {
    int count = 0;
    InstalledApp **apps = package_db_load(&count);
    if (!apps) return;

    InstalledApp **new_apps = malloc(sizeof(InstalledApp *) * count);
    int new_count = 0;

    for (int i = 0; i < count; i++) {
        if (apps[i]->full_name && strcmp(apps[i]->full_name, full_name) == 0) {
            installed_app_free(apps[i]);
        } else {
            new_apps[new_count++] = apps[i];
        }
    }

    package_db_save(new_apps, new_count);
    for (int i = 0; i < new_count; i++) installed_app_free(new_apps[i]);
    free(new_apps);
    free(apps);
}

InstalledApp *package_db_find(const char *full_name) {
    int count = 0;
    InstalledApp **apps = package_db_load(&count);
    if (!apps) return NULL;

    for (int i = 0; i < count; i++) {
        if (apps[i]->full_name && strcmp(apps[i]->full_name, full_name) == 0) {
            InstalledApp *found = apps[i];
            for (int j = 0; j < count; j++) if (j != i) installed_app_free(apps[j]);
            free(apps);
            return found;
        }
        installed_app_free(apps[i]);
    }
    free(apps);
    return NULL;
}

void package_db_clear(void) {
    package_db_save(NULL, 0);
}

void package_db_update_latest(const char *full_name, const char *latest_version, int has_update) {
    int count = 0;
    InstalledApp **apps = package_db_load(&count);
    if (!apps) return;

    for (int i = 0; i < count; i++) {
        if (apps[i]->full_name && strcmp(apps[i]->full_name, full_name) == 0) {
            free(apps[i]->latest_version);
            apps[i]->latest_version = latest_version ? strdup(latest_version) : NULL;
            apps[i]->has_update = has_update;
        }
    }

    package_db_save(apps, count);
    for (int i = 0; i < count; i++) installed_app_free(apps[i]);
    free(apps);
}
