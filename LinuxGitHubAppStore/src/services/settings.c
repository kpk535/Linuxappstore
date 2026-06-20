#include "settings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <json-c/json.h>
#include <unistd.h>
#include <sys/stat.h>

static const char* get_config_dir(void) {
    static char path[256] = {0};
    if (path[0] == 0) {
        const char *home = getenv("HOME");
        snprintf(path, sizeof(path), "%s/.config/linux-github-appstore", home ? home : ".");
    }
    return path;
}

static const char* get_settings_file(void) {
    static char path[256] = {0};
    if (path[0] == 0) {
        snprintf(path, sizeof(path), "%s/settings.json", get_config_dir());
    }
    return path;
}

AppSettings* settings_load(void) {
    AppSettings *settings = malloc(sizeof(AppSettings));
    settings->github_token = NULL;
    settings->dark_mode = 0;
    settings->auto_check_updates = 1;
    settings->download_folder = NULL;

    const char *file = get_settings_file();
    FILE *f = fopen(file, "r");
    if (!f) {
        settings->download_folder = strdup(getenv("HOME") ?: "/tmp");
        return settings;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = 0;
    fclose(f);

    struct json_object *root = json_tokener_parse(buffer);
    if (root) {
        struct json_object *tmp;
        if (json_object_object_get_ex(root, "github_token", &tmp)) {
            settings->github_token = strdup(json_object_get_string(tmp) ?: "");
        }
        if (json_object_object_get_ex(root, "dark_mode", &tmp)) {
            settings->dark_mode = json_object_get_int(tmp);
        }
        if (json_object_object_get_ex(root, "auto_check_updates", &tmp)) {
            settings->auto_check_updates = json_object_get_int(tmp);
        }
        if (json_object_object_get_ex(root, "download_folder", &tmp)) {
            settings->download_folder = strdup(json_object_get_string(tmp) ?: "");
        }
        json_object_put(root);
    }

    free(buffer);

    if (!settings->download_folder) {
        settings->download_folder = strdup(getenv("HOME") ?: "/tmp");
    }

    return settings;
}

void settings_save(AppSettings *settings) {
    mkdir(get_config_dir(), 0755);

    struct json_object *root = json_object_new_object();
    if (settings->github_token) {
        json_object_object_add(root, "github_token", json_object_new_string(settings->github_token));
    }
    json_object_object_add(root, "dark_mode", json_object_new_int(settings->dark_mode));
    json_object_object_add(root, "auto_check_updates", json_object_new_int(settings->auto_check_updates));
    if (settings->download_folder) {
        json_object_object_add(root, "download_folder", json_object_new_string(settings->download_folder));
    }

    const char *json_str = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY);

    FILE *f = fopen(get_settings_file(), "w");
    if (f) {
        fprintf(f, "%s", json_str);
        fclose(f);
    }

    json_object_put(root);
}

void settings_free(AppSettings *settings) {
    if (!settings) return;
    free(settings->github_token);
    free(settings->download_folder);
    free(settings);
}
