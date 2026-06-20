#include "app.h"
#include <stdlib.h>
#include <string.h>

GitHubRepository *repository_new(void) {
    GitHubRepository *repo = malloc(sizeof(GitHubRepository));
    if (repo) {
        repo->id = NULL;
        repo->name = NULL;
        repo->description = NULL;
        repo->url = NULL;
        repo->language = NULL;
        repo->stars = 0;
    }
    return repo;
}

void repository_free(GitHubRepository *repo) {
    if (repo) {
        free(repo->id);
        free(repo->name);
        free(repo->description);
        free(repo->url);
        free(repo->language);
        free(repo);
    }
}

InstalledApp *installed_app_new(void) {
    InstalledApp *app = malloc(sizeof(InstalledApp));
    if (app) {
        app->name = NULL;
        app->path = NULL;
        app->version = NULL;
    }
    return app;
}

void installed_app_free(InstalledApp *app) {
    if (app) {
        free(app->name);
        free(app->path);
        free(app->version);
        free(app);
    }
}
