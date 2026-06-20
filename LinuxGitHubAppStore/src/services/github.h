#ifndef GITHUB_SERVICE_H
#define GITHUB_SERVICE_H

#include "../models/app.h"

typedef struct {
    char *access_token;
} GitHubService;

GitHubService* github_service_new(const char *token);
void github_service_free(GitHubService *service);

typedef struct {
    GitHubRepository **items;
    int count;
} RepositoryList;

RepositoryList* github_search_repositories(GitHubService *service, const char *query, int page);
void repository_list_free(RepositoryList *list);

char* github_get_user_profile(GitHubService *service);

#endif
