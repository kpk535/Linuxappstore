#ifndef GITHUB_SERVICE_H
#define GITHUB_SERVICE_H

#include <stddef.h>
#include "../models/app.h"
#include "../models/release.h"

typedef struct {
    char *access_token;
} GitHubService;

typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

GitHubService *github_service_new(const char *token);
void github_service_free(GitHubService *service);

GitHubRepository **github_search_repositories(GitHubService *service, const char *query, int *count);
char *github_get_user_profile(GitHubService *service);
char *github_validate_token(GitHubService *service);

GitHubRelease **github_get_releases(GitHubService *service, const char *full_name, int *count);
GitHubRelease  *github_get_latest_release(GitHubService *service, const char *full_name);

#endif
