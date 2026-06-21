#ifndef APP_MODELS_H
#define APP_MODELS_H

typedef struct {
    char *id;
    char *name;
    char *full_name;
    char *description;
    char *url;
    char *language;
    int stars;
    int forks;
} GitHubRepository;

typedef struct {
    char *name;
    char *full_name;
    char *version;
    char *install_path;
    char *install_type;
    char *repo_url;
    char *installed_at;
    char *latest_version;
    int has_update;
} InstalledApp;

GitHubRepository *repository_new(void);
void repository_free(GitHubRepository *repo);

InstalledApp *installed_app_new(void);
void installed_app_free(InstalledApp *app);

#endif
