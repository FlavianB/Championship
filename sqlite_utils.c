#include "sqlite_utils.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void db_open(const char *db_name, sqlite3 **db) {
    int rc = sqlite3_open_v2(db_name, db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);

    if (rc != SQLITE_OK) {
        printf("Cannot open database: %s\n", sqlite3_errmsg(*db));
        sqlite3_close(*db);
        exit(1);
    }
}

void db_create(sqlite3 *db) {
    char sql[1024];

    sprintf(sql, "DROP TABLE IF EXISTS Users;"
                 "CREATE TABLE Users(u_id INT, username VARCHAR2, password VARCHAR2, email VARCHAR2);");
    sqlite3_exec(db, sql, NULL, NULL, NULL);

    sprintf(sql, "DROP TABLE IF EXISTS Tournaments;"
                 "CREATE TABLE Tournaments(t_id INT PRIMARY KEY, admin_id INT, name VARCHAR2, start_date DATETIME, game VARCHAR2, no_players_max INT, no_players_actual INT, structure INT, winner VARCHAR2, FOREIGN KEY (admin_id) REFERENCES Users (u_id), CHECK (no_players_max >= no_players_actual));");
    sqlite3_exec(db, sql, NULL, NULL, NULL);

    sprintf(sql, "DROP TABLE IF EXISTS Matches;");
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    sprintf(sql, "CREATE TABLE Matches(m_id, t_id INT, op1_id INT, op2_id INT, start_date DATETIME, result INT, FOREIGN KEY (t_id) REFERENCES Tournaments (t_id), FOREIGN KEY (op1_id) REFERENCES Users (u_id), FOREIGN KEY (op2_id) REFERENCES Users (u_id));");
    sqlite3_exec(db, sql, NULL, NULL, NULL);
}
