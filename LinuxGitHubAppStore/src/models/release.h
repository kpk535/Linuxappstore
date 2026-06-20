#ifndef RELEASE_MODEL_H
#define RELEASE_MODEL_H

#include <stddef.h>

typedef struct {
    char *name;
    char *download_url;
    long size;
} ReleaseAsset;

typedef struct {
    char *tag_name;
    char *name;
    char *html_url;
    char *published_at;
    ReleaseAsset **assets;
    int asset_count;
} GitHubRelease;

ReleaseAsset *release_asset_new(void);
void release_asset_free(ReleaseAsset *asset);
GitHubRelease *release_new(void);
void release_free(GitHubRelease *release);

#endif
