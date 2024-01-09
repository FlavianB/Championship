#ifndef SQLITE_UTILS_H
#define SQLITE_UTILS_H

#include <sqlite3.h>

#define CHAMP_DB "championship.db"
#define MAX_SQL_LEN 256

// Open database
void db_open(const char *db_name, sqlite3 **db);
// Create database
void db_create(sqlite3 *db);

#endif // SQLITE_UTILS_H
