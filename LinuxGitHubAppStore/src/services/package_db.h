#ifndef PACKAGE_DB_H
#define PACKAGE_DB_H

#include "../models/app.h"

InstalledApp **package_db_load(int *count);
void package_db_save(InstalledApp **apps, int count);
void package_db_add(InstalledApp *app);
void package_db_remove(const char *full_name);
InstalledApp *package_db_find(const char *full_name);
void package_db_update_latest(const char *full_name, const char *latest_version, int has_update);

#endif
