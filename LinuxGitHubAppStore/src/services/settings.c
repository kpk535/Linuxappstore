#include "settings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <json-c/json.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CONFIG_DIR ".config/linux-github-appstore"
#define CONFIG_FILE "settings.json"

static char *get_config_path(void) {
    const char *home = getenv("HOME");
    if (!home) home = "/root";

    static char path[512];
    snprintf(path, sizeof(path), "%s/%s/%s", home, CONFIG_DIR, CONFIG_FILE);
    return path;
}

static void ensure_config_dir(void) {
    const char *home = getenv("HOME");
    if (!home) home = "/root";

    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/%s", home, CONFIG_DIR);

    mkdir(dir_path, 0755);
}

AppSettings *settings_new(void) {
    AppSettings *settings = malloc(sizeof(AppSettings));
    if (settings) {
        settings->github_token = NULL;
        settings->dark_mode = 0;
        settings->auto_check_updates = 1;
        settings->download_folder = strdup("");
    }
    return settings;
}

void settings_free(AppSettings *settings) {
    if (settings) {
        free(settings->github_token);
        free(settings->download_folder);
        free(settings);
    }
}

AppSettings *settings_load(void) {
    AppSettings *settings = settings_new();

    char *path = get_config_path();
    FILE *f = fopen(path, "r");

    if (!f) {
        return settings;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(fsize + 1);
    fread(content, 1, fsize, f);
    fclose(f);
    content[fsize] = 0;

    json_object *parsed_json = json_tokener_parse(content);
    if (parsed_json) {
        json_object *obj;

        if (json_object_object_get_ex(parsed_json, "github_token", &obj)) {
            free(settings->github_token);
            settings->github_token = strdup(json_object_get_string(obj));
        }
        if (json_object_object_get_ex(parsed_json, "dark_mode", &obj)) {
            settings->dark_mode = json_object_get_boolean(obj);
        }
        if (json_object_object_get_ex(parsed_json, "auto_check_updates", &obj)) {
            settings->auto_check_updates = json_object_get_boolean(obj);
        }
        if (json_object_object_get_ex(parsed_json, "download_folder", &obj)) {
            free(settings->download_folder);
            settings->download_folder = strdup(json_object_get_string(obj));
        }

        json_object_put(parsed_json);
    }

    free(content);
    return settings;
}

void settings_save(AppSettings *settings) {
    if (!settings) return;

    ensure_config_dir();

    json_object *json = json_object_new_object();

    if (settings->github_token) {
        json_object_object_add(json, "github_token", json_object_new_string(settings->github_token));
    }
    json_object_object_add(json, "dark_mode", json_object_new_boolean(settings->dark_mode));
    json_object_object_add(json, "auto_check_updates", json_object_new_boolean(settings->auto_check_updates));
    if (settings->download_folder) {
        json_object_object_add(json, "download_folder", json_object_new_string(settings->download_folder));
    }

    const char *json_str = json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY);

    char *path = get_config_path();
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s\n", json_str);
        fclose(f);
    }

    json_object_put(json);
}
