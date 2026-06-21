#ifndef GITHUB_SERVICE_H
#define GITHUB_SERVICE_H

#include <stddef.h>
#include "../models/app.h"
#include "../models/release.h"

typedef struct {
    char *access_token;
    /* error state from last operation */
    long  last_http_code;
    char *last_error;
    int   rate_limit_remaining;   /* -1 = unknown */
    long  rate_limit_reset;       /* Unix timestamp, 0 = unknown */
} GitHubService;

typedef struct {
    char   *data;
    size_t  size;
} ResponseBuffer;

GitHubService *github_service_new(const char *token);
void           github_service_free(GitHubService *service);

/* Human-readable error from last HTTP code. */
const char *github_strerror(long http_code);
int         github_is_rate_limited(GitHubService *svc);
char       *github_rate_limit_message(GitHubService *svc); /* caller frees */

GitHubRepository **github_search_repositories(GitHubService *service,
                                              const char *query, int *count);
char  *github_get_user_profile(GitHubService *service);
char  *github_validate_token(GitHubService *service);

GitHubRelease **github_get_releases(GitHubService *service,
                                    const char *full_name, int *count);
GitHubRelease  *github_get_latest_release(GitHubService *service,
                                          const char *full_name);

#endif
