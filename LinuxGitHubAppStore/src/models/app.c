#include "app.h"
#include <stdlib.h>
#include <string.h>

void repository_free(GitHubRepository *repo) {
    if (!repo) return;
    free(repo->id);
    free(repo->name);
    free(repo->full_name);
    free(repo->description);
    free(repo->html_url);
    free(repo->clone_url);
    free(repo->zip_url);
    free(repo->language);
    free(repo->owner_login);
    free(repo->owner_avatar_url);
    free(repo);
}

void installed_app_free(InstalledApp *app) {
    if (!app) return;
    free(app->name);
    free(app->full_name);
    free(app->version);
    free(app->description);
    free(app->source_url);
    free(app->avatar_url);
    free(app->install_path);
    free(app);
}
