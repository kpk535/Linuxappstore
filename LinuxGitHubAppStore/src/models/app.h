#ifndef APP_MODELS_H
#define APP_MODELS_H

typedef struct {
    char *id;
    char *name;
    char *description;
    char *url;
    char *language;
    int stars;
} GitHubRepository;

typedef struct {
    char *name;
    char *path;
    char *version;
} InstalledApp;

GitHubRepository *repository_new(void);
void repository_free(GitHubRepository *repo);

InstalledApp *installed_app_new(void);
void installed_app_free(InstalledApp *app);

#endif
