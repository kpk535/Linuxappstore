#include "release.h"
#include <stdlib.h>
#include <string.h>

ReleaseAsset *release_asset_new(void) {
    ReleaseAsset *a = calloc(1, sizeof(ReleaseAsset));
    return a;
}

void release_asset_free(ReleaseAsset *asset) {
    if (!asset) return;
    free(asset->name);
    free(asset->download_url);
    free(asset);
}

GitHubRelease *release_new(void) {
    GitHubRelease *r = calloc(1, sizeof(GitHubRelease));
    return r;
}

void release_free(GitHubRelease *release) {
    if (!release) return;
    free(release->tag_name);
    free(release->name);
    free(release->html_url);
    free(release->published_at);
    for (int i = 0; i < release->asset_count; i++) {
        release_asset_free(release->assets[i]);
    }
    free(release->assets);
    free(release);
}
