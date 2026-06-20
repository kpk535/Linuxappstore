#ifndef SETTINGS_SERVICE_H
#define SETTINGS_SERVICE_H

typedef struct {
    char *github_token;
    int dark_mode;
    int auto_check_updates;
    char *download_folder;
} AppSettings;

AppSettings* settings_load(void);
void settings_save(AppSettings *settings);
void settings_free(AppSettings *settings);

#endif
