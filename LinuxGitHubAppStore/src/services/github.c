#include "github.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>

/* ── curl helpers ──────────────────────────────────────────── */

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

static CURL *make_curl(const char *url, ResponseBuffer *resp,
                       struct curl_slist **out_headers, const char *token) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    struct curl_slist *h = NULL;
    h = curl_slist_append(h, "Accept: application/vnd.github+json");
    if (token) {
        char auth[300];
        snprintf(auth, sizeof(auth), "Authorization: token %s", token);
        h = curl_slist_append(h, auth);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    *out_headers = h;
    return curl;
}

static char *do_get(const char *url, const char *token) {
    ResponseBuffer resp = {0};
    resp.data = malloc(1);
    struct curl_slist *h = NULL;
    CURL *curl = make_curl(url, &resp, &h, token);
    if (!curl) { free(resp.data); return NULL; }
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(h);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) { free(resp.data); return NULL; }
    return resp.data;
}

/* ── service lifecycle ─────────────────────────────────────── */

GitHubService *github_service_new(const char *token) {
    GitHubService *svc = malloc(sizeof(GitHubService));
    svc->access_token = token ? strdup(token) : NULL;
    return svc;
}

void github_service_free(GitHubService *svc) {
    if (!svc) return;
    free(svc->access_token);
    free(svc);
}

/* ── search repositories ───────────────────────────────────── */

GitHubRepository **github_search_repositories(GitHubService *svc,
                                              const char *query, int *count) {
    if (!query || !count) return NULL;
    *count = 0;

    CURL *tmp = curl_easy_init();
    char *escaped = curl_easy_escape(tmp, query, 0);
    char url[512];
    snprintf(url, sizeof(url),
             "https://api.github.com/search/repositories?q=%s&sort=stars&order=desc&per_page=20",
             escaped);
    curl_free(escaped);
    curl_easy_cleanup(tmp);

    char *json = do_get(url, svc ? svc->access_token : NULL);
    if (!json) return NULL;

    json_object *root = json_tokener_parse(json);
    free(json);
    if (!root) return NULL;

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
                r->language = l ? strdup(l) : strdup("N/A");
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
    }

    json_object_put(root);
    return repos;
}

/* ── user profile ──────────────────────────────────────────── */

char *github_get_user_profile(GitHubService *svc) {
    if (!svc || !svc->access_token)
        return strdup("{\"login\":\"anonymous\",\"name\":\"Not signed in\"}");
    char *data = do_get("https://api.github.com/user", svc->access_token);
    return data ? data : strdup("{\"login\":\"error\",\"name\":\"Failed to load\"}");
}

/* ── token validation ──────────────────────────────────────── */

char *github_validate_token(GitHubService *svc) {
    if (!svc || !svc->access_token) return strdup("false");

    ResponseBuffer resp = {0};
    resp.data = malloc(1);
    struct curl_slist *h = NULL;
    CURL *curl = make_curl("https://api.github.com/user", &resp, &h, svc->access_token);
    if (!curl) { free(resp.data); return strdup("false"); }
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_slist_free_all(h);
    free(resp.data);
    curl_easy_cleanup(curl);
    return strdup(code == 200 ? "true" : "false");
}

/* ── releases ──────────────────────────────────────────────── */

static GitHubRelease *parse_release(json_object *obj) {
    GitHubRelease *r = release_new();
    json_object *v;

    if (json_object_object_get_ex(obj, "tag_name", &v))
        r->tag_name = strdup(json_object_get_string(v));
    if (json_object_object_get_ex(obj, "name", &v))
        r->name = strdup(json_object_get_string(v));
    if (json_object_object_get_ex(obj, "html_url", &v))
        r->html_url = strdup(json_object_get_string(v));
    if (json_object_object_get_ex(obj, "published_at", &v))
        r->published_at = strdup(json_object_get_string(v));

    json_object *assets;
    if (json_object_object_get_ex(obj, "assets", &assets)) {
        int n = json_object_array_length(assets);
        r->assets = malloc(sizeof(ReleaseAsset *) * (n + 1));
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

    char *json = do_get(url, svc ? svc->access_token : NULL);
    if (!json) return NULL;

    json_object *root = json_tokener_parse(json);
    free(json);
    if (!root) return NULL;

    if (!json_object_is_type(root, json_type_array)) {
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

GitHubRelease *github_get_latest_release(GitHubService *svc, const char *full_name) {
    char url[256];
    snprintf(url, sizeof(url),
             "https://api.github.com/repos/%s/releases/latest", full_name);
    char *json = do_get(url, svc ? svc->access_token : NULL);
    if (!json) return NULL;
    json_object *root = json_tokener_parse(json);
    free(json);
    if (!root) return NULL;
    GitHubRelease *r = parse_release(root);
    json_object_put(root);
    return r;
}
