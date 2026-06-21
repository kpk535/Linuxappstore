#include "app.h"
#include <stdlib.h>
#include <string.h>

GitHubRepository *repository_new(void) {
    GitHubRepository *repo = calloc(1, sizeof(GitHubRepository));
    return repo;
}

void repository_free(GitHubRepository *repo) {
    if (!repo) return;
    free(repo->id);
    free(repo->name);
    free(repo->full_name);
    free(repo->description);
    free(repo->url);
    free(repo->language);
    free(repo);
}

InstalledApp *installed_app_new(void) {
    InstalledApp *app = calloc(1, sizeof(InstalledApp));
    return app;
}

void installed_app_free(InstalledApp *app) {
    if (!app) return;
    free(app->name);
    free(app->full_name);
    free(app->version);
    free(app->install_path);
    free(app->install_type);
    free(app->repo_url);
    free(app->installed_at);
    free(app->latest_version);
    free(app);
}
