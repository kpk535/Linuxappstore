#ifndef APP_H
#define APP_H

typedef struct {
    char *id;
    char *name;
    char *full_name;
    char *description;
    char *html_url;
    char *clone_url;
    char *zip_url;
    char *language;
    int stars;
    int forks;
    char *owner_login;
    char *owner_avatar_url;
} GitHubRepository;

typedef struct {
    char *name;
    char *full_name;
    char *version;
    char *description;
    char *source_url;
    char *avatar_url;
    char *install_path;
    int installed;
} InstalledApp;

void repository_free(GitHubRepository *repo);
void installed_app_free(InstalledApp *app);

#endif
