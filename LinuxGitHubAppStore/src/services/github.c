#include "github.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include <json-c/json.h>

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *mem = (ResponseBuffer *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "Not enough memory for response\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

GitHubService *github_service_new(const char *token) {
    GitHubService *service = malloc(sizeof(GitHubService));
    if (service) {
        service->access_token = token ? strdup(token) : NULL;
    }
    return service;
}

void github_service_free(GitHubService *service) {
    if (service) {
        free(service->access_token);
        free(service);
    }
}

GitHubRepository **github_search_repositories(GitHubService *service, const char *query, int *count) {
    if (!query || !count) return NULL;

    *count = 0;

    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer response = {0};
    response.data = malloc(1);

    char url[512];
    snprintf(url, sizeof(url), "https://api.github.com/search/repositories?q=%s&sort=stars&order=desc&per_page=10", query);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

    if (service && service->access_token) {
        char auth[256];
        snprintf(auth, sizeof(auth), "token %s", service->access_token);
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
        curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, service->access_token);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, auth);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(response.data);
        curl_easy_cleanup(curl);
        return NULL;
    }

    GitHubRepository **repos = NULL;
    json_object *parsed_json = json_tokener_parse(response.data);

    if (parsed_json) {
        json_object *items;
        if (json_object_object_get_ex(parsed_json, "items", &items)) {
            int arr_len = json_object_array_length(items);
            repos = malloc(sizeof(GitHubRepository *) * arr_len);

            for (int i = 0; i < arr_len; i++) {
                json_object *item = json_object_array_get_idx(items, i);
                repos[i] = repository_new();

                json_object *obj;
                if (json_object_object_get_ex(item, "name", &obj)) {
                    repos[i]->name = strdup(json_object_get_string(obj));
                }
                if (json_object_object_get_ex(item, "description", &obj)) {
                    const char *desc = json_object_get_string(obj);
                    repos[i]->description = desc ? strdup(desc) : strdup("");
                }
                if (json_object_object_get_ex(item, "html_url", &obj)) {
                    repos[i]->url = strdup(json_object_get_string(obj));
                }
                if (json_object_object_get_ex(item, "language", &obj)) {
                    const char *lang = json_object_get_string(obj);
                    repos[i]->language = lang ? strdup(lang) : strdup("N/A");
                }
                if (json_object_object_get_ex(item, "stargazers_count", &obj)) {
                    repos[i]->stars = json_object_get_int(obj);
                }
                if (json_object_object_get_ex(item, "id", &obj)) {
                    char id_str[32];
                    snprintf(id_str, sizeof(id_str), "%d", json_object_get_int(obj));
                    repos[i]->id = strdup(id_str);
                }
            }
            *count = arr_len;
        }
        json_object_put(parsed_json);
    }

    free(response.data);
    curl_easy_cleanup(curl);

    return repos;
}

char *github_get_user_profile(GitHubService *service) {
    if (!service || !service->access_token) {
        return strdup("{\"login\":\"anonymous\",\"name\":\"Not signed in\"}");
    }

    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer response = {0};
    response.data = malloc(1);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/user");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

    char auth[256];
    snprintf(auth, sizeof(auth), "token %s", service->access_token);
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(response.data);
        return strdup("{\"login\":\"error\",\"name\":\"Failed to load profile\"}");
    }

    char *result = response.data;
    return result;
}

char *github_validate_token(GitHubService *service) {
    if (!service || !service->access_token) {
        return strdup("false");
    }

    CURL *curl = curl_easy_init();
    if (!curl) return strdup("false");

    ResponseBuffer response = {0};
    response.data = malloc(1);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/user");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "LinuxGitHubAppStore/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    char auth[256];
    snprintf(auth, sizeof(auth), "token %s", service->access_token);
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, auth);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    free(response.data);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK && http_code == 200) {
        return strdup("true");
    }

    return strdup("false");
}
