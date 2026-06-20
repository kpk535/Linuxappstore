#include "github.h"
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;
    char *ptr = realloc(buf->data, buf->size + realsize + 1);
    if (!ptr) return 0;
    buf->data = ptr;
    memcpy(&(buf->data[buf->size]), contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = 0;
    return realsize;
}

GitHubService* github_service_new(const char *token) {
    GitHubService *service = malloc(sizeof(GitHubService));
    service->access_token = token ? strdup(token) : NULL;
    return service;
}

void github_service_free(GitHubService *service) {
    if (!service) return;
    free(service->access_token);
    free(service);
}

RepositoryList* github_search_repositories(GitHubService *service, const char *query, int page) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    RepositoryList *list = malloc(sizeof(RepositoryList));
    list->items = NULL;
    list->count = 0;

    char url[512];
    snprintf(url, sizeof(url),
             "https://api.github.com/search/repositories?q=%s&page=%d&per_page=30&sort=stars",
             query, page);

    ResponseBuffer buf = {0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");

    if (service->access_token) {
        char auth[256];
        snprintf(auth, sizeof(auth), "Authorization: token %s", service->access_token);
        headers = curl_slist_append(headers, auth);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.0");

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK && buf.data) {
        struct json_object *root = json_tokener_parse(buf.data);
        if (root) {
            struct json_object *items_obj;
            if (json_object_object_get_ex(root, "items", &items_obj) &&
                json_object_is_type(items_obj, json_type_array)) {

                int count = json_object_array_length(items_obj);
                list->items = malloc(count * sizeof(GitHubRepository*));

                for (int i = 0; i < count; i++) {
                    struct json_object *item = json_object_array_get_idx(items_obj, i);
                    GitHubRepository *repo = malloc(sizeof(GitHubRepository));

                    struct json_object *tmp;
                    repo->id = json_object_object_get_ex(item, "id", &tmp) ?
                               strdup(json_object_get_string(tmp)) : strdup("");
                    repo->name = json_object_object_get_ex(item, "name", &tmp) ?
                                 strdup(json_object_get_string(tmp)) : strdup("");
                    repo->full_name = json_object_object_get_ex(item, "full_name", &tmp) ?
                                      strdup(json_object_get_string(tmp)) : strdup("");
                    repo->description = json_object_object_get_ex(item, "description", &tmp) ?
                                        strdup(json_object_get_string(tmp) ?: "") : strdup("");
                    repo->html_url = json_object_object_get_ex(item, "html_url", &tmp) ?
                                     strdup(json_object_get_string(tmp)) : strdup("");
                    repo->clone_url = json_object_object_get_ex(item, "clone_url", &tmp) ?
                                      strdup(json_object_get_string(tmp)) : strdup("");
                    repo->zip_url = json_object_object_get_ex(item, "zipball_url", &tmp) ?
                                    strdup(json_object_get_string(tmp)) : strdup("");
                    repo->language = json_object_object_get_ex(item, "language", &tmp) ?
                                     strdup(json_object_get_string(tmp) ?: "Unknown") : strdup("Unknown");
                    repo->stars = json_object_object_get_ex(item, "stargazers_count", &tmp) ?
                                  json_object_get_int(tmp) : 0;
                    repo->forks = json_object_object_get_ex(item, "forks_count", &tmp) ?
                                  json_object_get_int(tmp) : 0;

                    struct json_object *owner;
                    if (json_object_object_get_ex(item, "owner", &owner)) {
                        repo->owner_login = json_object_object_get_ex(owner, "login", &tmp) ?
                                            strdup(json_object_get_string(tmp)) : strdup("");
                        repo->owner_avatar_url = json_object_object_get_ex(owner, "avatar_url", &tmp) ?
                                                 strdup(json_object_get_string(tmp)) : strdup("");
                    } else {
                        repo->owner_login = strdup("");
                        repo->owner_avatar_url = strdup("");
                    }

                    list->items[i] = repo;
                }
                list->count = count;
            }
            json_object_put(root);
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(buf.data);

    return list;
}

void repository_list_free(RepositoryList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        repository_free(list->items[i]);
    }
    free(list->items);
    free(list);
}

char* github_get_user_profile(GitHubService *service) {
    if (!service || !service->access_token) return NULL;

    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer buf = {0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");

    char auth[256];
    snprintf(auth, sizeof(auth), "Authorization: token %s", service->access_token);
    headers = curl_slist_append(headers, auth);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/user");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.0");

    curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return buf.data;
}
