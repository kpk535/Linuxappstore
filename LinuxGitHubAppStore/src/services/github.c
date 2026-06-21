#include "github.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <curl/curl.h>
#include <json-c/json.h>

/* ── write callback ───────────────────────────────────────────── */

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *mem = (ResponseBuffer *)userp;
    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) return 0;
    mem->data = ptr;
    memcpy(&mem->data[mem->size], contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    return realsize;
}

/* ── header callback: parse rate-limit headers ─────────────────── */

static size_t header_callback(char *buf, size_t size, size_t nitems, void *userp) {
    size_t len = size * nitems;
    GitHubService *svc = (GitHubService *)userp;
    if (!svc) return len;

    if (strncasecmp(buf, "X-RateLimit-Remaining:", 22) == 0)
        svc->rate_limit_remaining = atoi(buf + 22);
    else if (strncasecmp(buf, "X-RateLimit-Reset:", 18) == 0)
        svc->rate_limit_reset = atol(buf + 18);
    return len;
}

/* ── HTTP error helper ────────────────────────────────────────── */

const char *github_strerror(long code) {
    switch (code) {
        case 0:   return "Network error — check your internet connection";
        case 200: return NULL;
        case 201: return NULL;
        case 204: return NULL;
        case 301: return "Resource moved";
        case 400: return "Bad request — malformed query";
        case 401: return "Unauthorized — your GitHub token is invalid or expired";
        case 403: return "Forbidden — token lacks required permissions or rate limit hit";
        case 404: return "Not found — repository or resource does not exist";
        case 409: return "Conflict";
        case 422: return "Unprocessable entity — invalid search query";
        case 429: return "Rate limit exceeded — please wait before retrying";
        case 500: return "GitHub server error — try again later";
        case 502: return "GitHub bad gateway — try again later";
        case 503: return "GitHub service unavailable — try again later";
        default:  return "Unexpected HTTP error";
    }
}

int github_is_rate_limited(GitHubService *svc) {
    if (!svc) return 0;
    return (svc->last_http_code == 403 || svc->last_http_code == 429)
           && svc->rate_limit_remaining == 0;
}

char *github_rate_limit_message(GitHubService *svc) {
    if (!svc || svc->rate_limit_remaining < 0) return NULL;
    char msg[128];
    if (svc->rate_limit_reset > 0) {
        long now   = (long)time(NULL);
        long secs  = svc->rate_limit_reset - now;
        if (secs < 0) secs = 0;
        snprintf(msg, sizeof(msg),
                 "API: %d requests left  (resets in %ldm %lds)",
                 svc->rate_limit_remaining, secs / 60, secs % 60);
    } else {
        snprintf(msg, sizeof(msg), "API: %d requests left",
                 svc->rate_limit_remaining);
    }
    return strdup(msg);
}

/* ── curl request ─────────────────────────────────────────────── */

static char *do_get(GitHubService *svc, const char *url) {
    ResponseBuffer resp = {0};
    resp.data = malloc(1);
    resp.data[0] = '\0';

    CURL *curl = curl_easy_init();
    if (!curl) { free(resp.data); return NULL; }

    struct curl_slist *h = NULL;
    h = curl_slist_append(h, "Accept: application/vnd.github+json");
    h = curl_slist_append(h, "X-GitHub-Api-Version: 2022-11-28");
    if (svc && svc->access_token) {
        char auth[320];
        snprintf(auth, sizeof(auth), "Authorization: token %s", svc->access_token);
        h = curl_slist_append(h, auth);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.1");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, svc);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(h);
    curl_easy_cleanup(curl);

    if (svc) {
        svc->last_http_code = http_code;
        free(svc->last_error);
        svc->last_error = NULL;
    }

    if (res != CURLE_OK) {
        /* Network-level failure — retry once after a short pause */
        free(resp.data);
        resp.data = malloc(1);
        resp.data[0] = '\0';
        resp.size  = 0;

        curl = curl_easy_init();
        if (!curl) { free(resp.data); return NULL; }

        h = NULL;
        h = curl_slist_append(h, "Accept: application/vnd.github+json");
        h = curl_slist_append(h, "X-GitHub-Api-Version: 2022-11-28");
        if (svc && svc->access_token) {
            char auth[320];
            snprintf(auth, sizeof(auth), "Authorization: token %s", svc->access_token);
            h = curl_slist_append(h, auth);
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.1");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, svc);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(h);
        curl_easy_cleanup(curl);

        if (svc) svc->last_http_code = http_code;

        if (res != CURLE_OK) {
            if (svc) {
                svc->last_error = strdup(curl_easy_strerror(res));
            }
            free(resp.data);
            return NULL;
        }
    }

    /* HTTP-level errors */
    if (http_code >= 400) {
        if (svc) {
            const char *msg = github_strerror(http_code);
            svc->last_error = msg ? strdup(msg) : NULL;
        }
        free(resp.data);
        return NULL;
    }

    return resp.data;
}

/* ── service lifecycle ─────────────────────────────────────────── */

GitHubService *github_service_new(const char *token) {
    GitHubService *svc = calloc(1, sizeof(GitHubService));
    svc->access_token       = token ? strdup(token) : NULL;
    svc->rate_limit_remaining = -1;
    return svc;
}

void github_service_free(GitHubService *svc) {
    if (!svc) return;
    free(svc->access_token);
    free(svc->last_error);
    free(svc);
}

/* ── search repositories ───────────────────────────────────────── */

GitHubRepository **github_search_repositories(GitHubService *svc,
                                              const char *query, int *count) {
    if (!query || !count) return NULL;
    *count = 0;

    CURL *tmp = curl_easy_init();
    char *escaped = curl_easy_escape(tmp, query, 0);
    char url[512];
    snprintf(url, sizeof(url),
             "https://api.github.com/search/repositories"
             "?q=%s&sort=stars&order=desc&per_page=25",
             escaped);
    curl_free(escaped);
    curl_easy_cleanup(tmp);

    char *json = do_get(svc, url);
    if (!json) return NULL;

    json_object *root = json_tokener_parse(json);
    free(json);
    if (!root) {
        if (svc) { free(svc->last_error); svc->last_error = strdup("Invalid JSON response"); }
        return NULL;
    }

    /* Check for API error message inside JSON */
    json_object *msg_obj;
    if (json_object_object_get_ex(root, "message", &msg_obj)) {
        if (svc) {
            free(svc->last_error);
            svc->last_error = strdup(json_object_get_string(msg_obj));
        }
        json_object_put(root);
        return NULL;
    }

    GitHubRepository **repos = NULL;
    json_object *items;
    if (json_object_object_get_ex(root, "items", &items)) {
        int len = json_object_array_length(items);
        repos = malloc(sizeof(GitHubRepository *) * (len + 1));

        for (int i = 0; i < len; i++) {
            json_object *item = json_object_array_get_idx(items, i);
            GitHubRepository *r = repository_new();
            json_object *v;

            if (json_object_object_get_ex(item, "name", &v))
                r->name = strdup(json_object_get_string(v));
            if (json_object_object_get_ex(item, "full_name", &v))
                r->full_name = strdup(json_object_get_string(v));
            if (json_object_object_get_ex(item, "description", &v)) {
                const char *d = json_object_get_string(v);
                r->description = d ? strdup(d) : strdup("");
            }
            if (json_object_object_get_ex(item, "html_url", &v))
                r->url = strdup(json_object_get_string(v));
            if (json_object_object_get_ex(item, "language", &v)) {
                const char *l = json_object_get_string(v);
                r->language = l ? strdup(l) : NULL;
            }
            if (json_object_object_get_ex(item, "stargazers_count", &v))
                r->stars = json_object_get_int(v);
            if (json_object_object_get_ex(item, "forks_count", &v))
                r->forks = json_object_get_int(v);
            if (json_object_object_get_ex(item, "id", &v)) {
                char id_str[32];
                snprintf(id_str, sizeof(id_str), "%d", json_object_get_int(v));
                r->id = strdup(id_str);
            }
            repos[i] = r;
        }
        repos[len] = NULL;
        *count = len;
    } else {
        if (svc) { free(svc->last_error); svc->last_error = strdup("Unexpected response format"); }
    }

    json_object_put(root);
    return repos;
}

/* ── user profile ──────────────────────────────────────────────── */

char *github_get_user_profile(GitHubService *svc) {
    if (!svc || !svc->access_token)
        return strdup("{\"error\":\"no_token\"}");
    char *data = do_get(svc, "https://api.github.com/user");
    if (!data) {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "{\"error\":\"%s\"}",
                 svc->last_error ? svc->last_error : "network_error");
        return strdup(buf);
    }
    return data;
}

/* ── token validation ──────────────────────────────────────────── */

char *github_validate_token(GitHubService *svc) {
    if (!svc || !svc->access_token) return strdup("false");

    ResponseBuffer resp = {0};
    resp.data = malloc(1);
    resp.data[0] = '\0';

    CURL *curl = curl_easy_init();
    if (!curl) { free(resp.data); return strdup("false"); }

    struct curl_slist *h = NULL;
    h = curl_slist_append(h, "Accept: application/vnd.github+json");
    char auth[320];
    snprintf(auth, sizeof(auth), "Authorization: token %s", svc->access_token);
    h = curl_slist_append(h, auth);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/user");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.1");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, svc);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_perform(curl);

    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    svc->last_http_code = code;
    curl_slist_free_all(h);
    free(resp.data);
    curl_easy_cleanup(curl);
    return strdup(code == 200 ? "true" : "false");
}

/* ── releases ──────────────────────────────────────────────────── */

static GitHubRelease *parse_release(json_object *obj) {
    GitHubRelease *r = release_new();
    json_object *v;

    if (json_object_object_get_ex(obj, "tag_name",     &v)) r->tag_name     = strdup(json_object_get_string(v));
    if (json_object_object_get_ex(obj, "name",         &v)) r->name         = strdup(json_object_get_string(v));
    if (json_object_object_get_ex(obj, "html_url",     &v)) r->html_url     = strdup(json_object_get_string(v));
    if (json_object_object_get_ex(obj, "published_at", &v)) r->published_at = strdup(json_object_get_string(v));

    json_object *assets;
    if (json_object_object_get_ex(obj, "assets", &assets)) {
        int n = json_object_array_length(assets);
        r->assets      = malloc(sizeof(ReleaseAsset *) * (n + 1));
        r->asset_count = 0;

        for (int i = 0; i < n; i++) {
            json_object *a = json_object_array_get_idx(assets, i);
            ReleaseAsset *asset = release_asset_new();
            if (json_object_object_get_ex(a, "name", &v))
                asset->name = strdup(json_object_get_string(v));
            if (json_object_object_get_ex(a, "browser_download_url", &v))
                asset->download_url = strdup(json_object_get_string(v));
            if (json_object_object_get_ex(a, "size", &v))
                asset->size = json_object_get_int64(v);
            r->assets[r->asset_count++] = asset;
        }
        r->assets[r->asset_count] = NULL;
    }
    return r;
}

GitHubRelease **github_get_releases(GitHubService *svc,
                                    const char *full_name, int *count) {
    *count = 0;
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.github.com/repos/%s/releases?per_page=10", full_name);

    char *json = do_get(svc, url);
    if (!json) return NULL;

    json_object *root = json_tokener_parse(json);
    free(json);
    if (!root) {
        if (svc) { free(svc->last_error); svc->last_error = strdup("Invalid JSON response"); }
        return NULL;
    }

    /* API may return an object with "message" on error */
    json_object *msg_obj;
    if (!json_object_is_type(root, json_type_array)) {
        if (json_object_object_get_ex(root, "message", &msg_obj) && svc) {
            free(svc->last_error);
            svc->last_error = strdup(json_object_get_string(msg_obj));
        }
        json_object_put(root);
        return NULL;
    }

    int len = json_object_array_length(root);
    GitHubRelease **releases = malloc(sizeof(GitHubRelease *) * (len + 1));
    for (int i = 0; i < len; i++)
        releases[i] = parse_release(json_object_array_get_idx(root, i));
    releases[len] = NULL;
    *count = len;
    json_object_put(root);
    return releases;
}

GitHubRelease *github_get_latest_release(GitHubService *svc,
                                         const char *full_name) {
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.github.com/repos/%s/releases/latest", full_name);
    char *json = do_get(svc, url);
    if (!json) return NULL;
    json_object *root = json_tokener_parse(json);
    free(json);
    if (!root) return NULL;

    /* 404 returns JSON object with "message" */
    json_object *msg_obj;
    if (json_object_object_get_ex(root, "message", &msg_obj)) {
        json_object_put(root);
        return NULL;
    }

    GitHubRelease *r = parse_release(root);
    json_object_put(root);
    return r;
}
