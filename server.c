#include "djb2.h"
#include "email_utils.h"
#include "sqlite_utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 2024

typedef struct thData {
    int idThread;
    int cl;
    int current_user_id;
} thData;
sqlite3 *champ_db;
int user_data[1000000];
int user_data_index = 0;

static void *treat(void *);
void answer(void *, sqlite3 *);

int process_req(void *, char *, int, sqlite3 *);
void recv_fmt(char *, int);

void validate_credentials(void *, char *, int, sqlite3 *);
int check_account(thData *, char *, char *, sqlite3 *);
int create_account(char *, char *, char *, sqlite3 *);
int generate_unique_u_id(sqlite3 *);

void create_admin(sqlite3 *);
static int admin_callback(void *, int, char **, char **);

void create_tournament(thData *, char *, int, sqlite3 *);
int generate_unique_t_id(sqlite3 *);
int generate_unique_m_id(sqlite3 *);

void join_tournament(thData *, char *, int, sqlite3 *);

void show_tournaments(void *, char *, int, sqlite3 *);
void show_matches(thData *, char *, int, sqlite3 *);
void show_history(void *, char *, int, sqlite3 *);
void postpone(void *, char *, int, sqlite3 *);

void send_response(int value, int fd) {
    int response = value;
    if (write(fd, &response, sizeof(int)) == -1) {
        perror("[SERVER]Error at writing to specified client fd.\n");
        exit(1);
    }
}

int main() {
    printf("[SERVER]Will open %s ...\n", CHAMP_DB);
    db_open(CHAMP_DB, &champ_db);
    db_create(champ_db);
    create_admin(champ_db);

    struct sockaddr_in server;
    struct sockaddr_in from;
    int nr;
    int sd;
    int pid;
    pthread_t th[1000000];
    int i = 0;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[SERVER]Error at socket.\n");
        return errno;
    }

    int optval = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("[SERVER]Error setting socket options.\n");
        return errno;
    }

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        perror("[SERVER]Error at bind.\n");
        return errno;
    }

    if (listen(sd, 100) == -1) {
        perror("[SERVER]Error at listen.\n");
        return errno;
    }

    while (1) {
        int client;
        thData *td;
        int length = sizeof(from);

        printf("[SERVER]We will wait at the port: %d\n", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0) {
            perror("[SERVER]Error at accept.\n");
            continue;
        }

        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &treat, td);
    }

    sqlite3_close(champ_db);
};

static void *treat(void *arg) {
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[SERVER][THREAD] - %d Waiting for command: \n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());

    answer((struct thData *)arg, champ_db);

    printf("[SERVER][THREAD] - %d Closed connection. User with id %d exited. \n", tdL.idThread, user_data[tdL.idThread]);
    close((intptr_t)arg);
    return (NULL);
};

void answer(void *arg, sqlite3 *champ_db) {
    int nr = 0, i = 0;
    struct thData tdL;
    char req[1000], username[256], password[256], c;
    tdL = *((struct thData *)arg);

    while (1) {
        recv_fmt(req, tdL.cl);
        printf("[SERVER][THREAD] - %d Received command: %s\n", tdL.idThread, req);
        if (process_req(arg, req, tdL.cl, champ_db) == 1)
            break;
    }
}

int process_req(void *arg, char *r, int fd, sqlite3 *champ_db) {
    int response = 0;
    char req[1000], type[4];
    memset(req, 0, 1000);
    strcpy(req, r);
    struct thData tdL = *((struct thData *)arg);

    strncpy(type, req, 3);

    if (strcmp(type, "VAL") == 0) {
        validate_credentials(arg, req, fd, champ_db);
    } else if (strcmp(type, "CRT") == 0) {
        create_tournament(arg, req, fd, champ_db);
    } else if (strcmp(type, "JNT") == 0)
        join_tournament(arg, req, fd, champ_db);
    else if (strcmp(type, "SHT") == 0) {
        show_tournaments(arg, req, fd, champ_db);
    } else if (strcmp(type, "SHM") == 0) {
        show_matches(arg, req, fd, champ_db);
    } else if (strcmp(type, "SHH") == 0) {
        show_history(arg, req, fd, champ_db);
    } else if (strcmp(type, "PST") == 0) {
        postpone(arg, req, fd, champ_db);
    } else if (strcmp(type, "EXT") == 0) {
        close(fd);
        return 1;
    }

    return 0;
}

void recv_fmt(char *req, int fd) {
    int i = 0;
    char c = 0;

    while (c != '\n') {
        if (read(fd, &c, 1) == -1) {
            perror("[SERVER]Error at reading from specified client fd.\n");
            exit(1);
        }

        if (c != '\n')
            req[i++] = c;
    }
    req[i] = '\0';
}

void validate_credentials(void *arg, char *req, int fd, sqlite3 *champ_db) {
    int response = 0;
    char *type = strtok(req, "_");
    char *option = strtok(NULL, "_");

    struct thData tdL = *((struct thData *)arg);

    if (strcmp(option, "L") == 0) {
        char *username = strtok(NULL, "_");
        char *password = strtok(NULL, "_");
        printf("[SERVER]Validating account!\n");
        response = check_account(arg, username, password, champ_db);
        write(fd, &response, 4);
        printf("[SERVER]Sent response! %d \n", response);
    } else if (strcmp(option, "R") == 0) {
        char *username = strtok(NULL, "_");
        char *password = strtok(NULL, "_");
        char *email = strtok(NULL, "_");
        printf("[SERVER]Registering user!\n");
        response = create_account(username, password, email, champ_db);
        write(fd, &response, 4);
        printf("[SERVER]Sent response! %d \n", response);
    }
}

static int admin_callback(void *NotUsed, int argc, char **argv, char **azColName) {
    *(int *)NotUsed = 1;
    return 0;
}

void create_admin(sqlite3 *user_db) {
    char *poll = "SELECT * FROM Users WHERE u_id=1;";
    int user_exists = 0;
    char *errMsg = 0;
    char hash_pass[256];
    sprintf(hash_pass, "%lu", hash("admin"));

    int rc = sqlite3_exec(user_db, poll, admin_callback, &user_exists, &errMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else if (user_exists == 0) {
        sqlite3_stmt *stmt;
        char *insert = "INSERT INTO Users VALUES(1, 'admin', ?, 'admin@admin.com');";
        rc = sqlite3_prepare_v2(user_db, insert, -1, &stmt, NULL);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(user_db));
            return;
        }

        rc = sqlite3_bind_text(stmt, 1, hash_pass, -1, SQLITE_STATIC);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
            return;
        }

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(user_db));
            return;
        }

        sqlite3_finalize(stmt);
    }
}

int check_account(thData *arg, char *username, char *password, sqlite3 *user_db) {
    char *poll = "SELECT * FROM Users WHERE username=? AND password=?;";
    int user_exists = 0;
    char *errMsg = 0;
    char hash_pass[256];
    sprintf(hash_pass, "%lu", hash(password));

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(user_db, poll, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_bind_text(stmt, 2, hash_pass, -1, SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return 1;
    }

    char *select_id = "SELECT u_id FROM Users WHERE username=?;";

    rc = sqlite3_prepare_v2(user_db, select_id, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        arg->current_user_id = sqlite3_column_int(stmt, 0);
        user_data[arg->idThread] = sqlite3_column_int(stmt, 0); // Set the current_user_id to the u_id of the logged-in user
        sqlite3_finalize(stmt);
        printf("User %d logged in successfully!\n", arg->current_user_id);
        return 0;
    }
}

int create_account(char *username, char *password, char *email, sqlite3 *user_db) {
    char *poll = "SELECT * FROM Users WHERE username=?;";
    int user_exists = 0;
    char *errMsg = 0;

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(user_db, poll, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        fprintf(stderr, "Username already exists!\n");
        return 1;
    }

    printf("Email: %s\n", email);

    char *select = "SELECT * FROM Users WHERE email = ?;";
    rc = sqlite3_prepare_v2(user_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        fprintf(stderr, "Email already exists.\n");
        sqlite3_finalize(stmt);
        return 2;
    }

    sqlite3_finalize(stmt);

    char hash_pass[256];
    sprintf(hash_pass, "%lu", hash(password));

    char *insert = "INSERT INTO Users (u_id, username, password, email) VALUES (?, ?, ?, ?);";
    rc = sqlite3_prepare_v2(user_db, insert, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    int id = generate_unique_u_id(user_db); // Function to generate unique ID

    rc = sqlite3_bind_int(stmt, 1, id);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_bind_text(stmt, 3, hash_pass, -1, SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_bind_text(stmt, 4, email, -1, SQLITE_STATIC);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(user_db));
        return 1;
    }

    sqlite3_finalize(stmt);
    send_email(email, "Welcome to Championship Manager!", "Your account has been created successfully! You can now login to your account and manage your championships. \nYou will be notified about future matches via email. \n\nSee you around, champ!");

    return 0;
}

void create_tournament(thData *arg, char *req, int fd, sqlite3 *champ_db) {
    sqlite3_stmt *stmt;
    char error[256];
    int rc;

    char *command = strtok(req, "_");
    char *tournament_name = strtok(NULL, "_");
    char *start_date = strtok(NULL, "_");
    char *tournament_game = strtok(NULL, "_");
    char *no_players_max_string = strtok(NULL, "_");
    char *structure_string = strtok(NULL, "_");
    int t_id = generate_unique_t_id(champ_db);

    // Verify if the tournament name is unique
    char select[256];
    sprintf(select, "SELECT * FROM Tournaments WHERE name = '%s';", tournament_name);

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot exec statement: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        send_response(1, fd);
        return;
    }

    sqlite3_finalize(stmt);

    send_response(0, fd);

    // Insert the tournament into the database
    char insert[256];
    memset(insert, 0, 256);
    sprintf(insert, "INSERT INTO Tournaments (t_id, admin_id, name, start_date, game, no_players_max, no_players_actual, structure) VALUES (%d, %d, '%s', '%s', '%s', %d, 0, %d);", t_id, user_data[arg->idThread], tournament_name, start_date, tournament_game, atoi(no_players_max_string), atoi(structure_string));

    rc = sqlite3_exec(champ_db, insert, NULL, NULL, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot exec statement: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    // memset(insert, 0, 256);
    // sprintf(insert, "INSERT INTO Matches (m_id, t_id, op1_id, op2_id, start_date, result) VALUES (%d, %d, NULL, NULL, '%s', NULL);", generate_unique_m_id(champ_db), t_id, start_date);

    // rc = sqlite3_exec(champ_db, insert, NULL, NULL, NULL);

    // if (rc != SQLITE_OK) {
    //     fprintf(stderr, "Cannot exec statement: %s\n", sqlite3_errmsg(champ_db));
    //     return;
    // }
}

void join_tournament(thData *arg, char *req, int fd, sqlite3 *champ_db) {
    int response;
    char *command = strtok(req, "_");
    char *tournament_name = strtok(NULL, "_");

    // Verify if the tournament exists
    char select[256];
    sprintf(select, "SELECT t_id FROM Tournaments WHERE name=?;");

    struct thData tdL = *((struct thData *)arg);

    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    sqlite3_bind_text(stmt, 1, tournament_name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        send_response(1, fd);
        return;
    }

    int t_id = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    memset(select, 0, 256);
    sprintf(select, "SELECT * FROM Matches WHERE t_id=%d AND op1_id=%d;", t_id, user_data[arg->idThread]);

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        send_response(2, fd);
        return;
    }

    // Update the number of players in the tournament
    char update[256];
    memset(update, 0, 256);
    sprintf(update, "UPDATE Tournaments SET no_players_actual = no_players_actual + 1 WHERE name='%s';", tournament_name);

    rc = sqlite3_exec(champ_db, update, NULL, NULL, NULL);

    if (rc != SQLITE_OK) {
        send_response(3, fd);
        return;
    }

    printf("Updated the number of players in the tournament!\n");

    // Extract the start date of the tournament
    memset(select, 0, 256);
    sprintf(select, "SELECT start_date FROM Tournaments WHERE name=?;");

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    sqlite3_bind_text(stmt, 1, tournament_name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    char start_date[40];
    sprintf(start_date, "%s", sqlite3_column_text(stmt, 0));

    sqlite3_finalize(stmt);

    printf("Extracted the start date of the tournament!\n");

    // Add opponent to match or create a new match
    int m_id = generate_unique_m_id(champ_db);

    memset(select, 0, 256);
    sprintf(select, "SELECT m_id FROM Matches WHERE op2_id IS NULL AND t_id=%d;", t_id);

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc == SQLITE_ROW) {
        char update[256];
        memset(update, 0, 256);
        sprintf(update, "UPDATE Matches SET op2_id = %d WHERE m_id = (SELECT m_id FROM Matches WHERE op2_id is NULL AND t_id=%d);", user_data[arg->idThread], t_id);
        sqlite3_exec(champ_db, update, NULL, NULL, NULL);

        printf("Updated the opponent of the match!\n");
    }

    if (rc != SQLITE_ROW) {
        char insert[256];
        memset(insert, 0, 256);
        sprintf(insert, "INSERT INTO Matches (m_id, t_id, op1_id, op2_id, start_date, result) VALUES (%d, %d, %d, NULL, '%s', NULL);", m_id, t_id, user_data[arg->idThread], start_date);
        sqlite3_exec(champ_db, insert, NULL, NULL, NULL);

        printf("Inserted a new match!\n");
    }

    // Send email to the user
    memset(select, 0, 256);
    sprintf(select, "SELECT email FROM Users WHERE u_id=%d;", user_data[arg->idThread]);

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    const char *email = sqlite3_column_text(stmt, 0);

    sqlite3_finalize(stmt);

    char content[256];
    memset(content, 0, 256);
    sprintf(content, "You have joined the tournament %s! \n\nYou will be notified about future matches via email. \n\nSee you around, champ!", tournament_name);

    send_email((char *)email, "You have joined a tournament!", content);

    printf("Sent email to the user!\n");

    send_response(0, fd);
}

void show_tournaments(void *arg, char *req, int fd, sqlite3 *champ_db) {
    char select[256];
    sprintf(select, "SELECT name FROM Tournaments WHERE no_players_actual < no_players_max;");
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        send_response(1, fd);
        return;
    }

    sqlite3_finalize(stmt);

    send_response(0, fd);

    memset(select, 0, 256);
    sprintf(select, "SELECT COUNT(name) FROM Tournaments;");

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    send_response(sqlite3_column_int(stmt, 0), fd);

    sqlite3_finalize(stmt);

    memset(select, 0, 256);
    sprintf(select, "SELECT name, game, start_date, no_players_actual, no_players_max FROM Tournaments WHERE no_players_actual < no_players_max;");

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement here: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    while (1) {
        rc = sqlite3_step(stmt);

        if (rc != SQLITE_ROW) {
            break;
        }

        const char *tournament_name = sqlite3_column_text(stmt, 0);
        write(fd, tournament_name, strlen(tournament_name));
        write(fd, "\n", 1);
        const char *tournament_game = sqlite3_column_text(stmt, 1);
        write(fd, tournament_game, strlen(tournament_game));
        write(fd, "\n", 1);
        const char *start_date = sqlite3_column_text(stmt, 2);
        write(fd, start_date, strlen(start_date));
        write(fd, "\n", 1);
        char no_players_actual[256];
        sprintf(no_players_actual, "%d", sqlite3_column_int(stmt, 3));
        write(fd, &no_players_actual, strlen(no_players_actual));
        write(fd, "\n", 1);
        char no_players_max[256];
        sprintf(no_players_max, "%d", sqlite3_column_int(stmt, 4));
        write(fd, &no_players_max, strlen(no_players_max));
        write(fd, "\n", 1);
    }

    return;
}

void show_matches(thData *arg, char *req, int fd, sqlite3 *champ_db) {
    char select[256];
    sprintf(select, "SELECT COUNT(m_id) FROM Matches WHERE ((op1_id=%d AND op2_id IS NOT NULL) OR op2_id=%d) AND result IS NULL;", user_data[arg->idThread], user_data[arg->idThread]);
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot exec statement: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        send_response(1, fd);
        return;
    }

    send_response(0, fd);

    sqlite3_finalize(stmt);

    memset(select, 0, 256);
    sprintf(select, "SELECT COUNT(t_id) FROM Matches WHERE ((op1_id=%d AND op2_id IS NOT NULL) OR op2_id=%d) AND result IS NULL;", user_data[arg->idThread], user_data[arg->idThread]);

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot exec statement: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        send_response(1, fd);
        return;
    }

    printf("Matches found: %d\n", sqlite3_column_int(stmt, 0));

    send_response(sqlite3_column_int(stmt, 0), fd);

    sqlite3_finalize(stmt);

    memset(select, 0, 256);
    sprintf(select, "SELECT m_id, username, m.start_date FROM Matches m JOIN Users u ON op2_id = u_id WHERE op1_id = %d;", user_data[arg->idThread]);

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot exec statement: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    while (1) {
        rc = sqlite3_step(stmt);

        if (rc != SQLITE_ROW) {
            break;
        }

        char m_id[256];
        sprintf(m_id, "%d", sqlite3_column_int(stmt, 0));
        write(fd, m_id, strlen(m_id));
        write(fd, "\n", 1);
        const char *opponent_name = sqlite3_column_text(stmt, 1);
        write(fd, opponent_name, strlen(opponent_name));
        write(fd, "\n", 1);
        const char *start_date = sqlite3_column_text(stmt, 2);
        write(fd, start_date, strlen(start_date));
        write(fd, "\n", 1);
    }

    sqlite3_finalize(stmt);

    memset(select, 0, 256);
    sprintf(select, "SELECT m_id, username, m.start_date FROM Matches m JOIN Users u ON op1_id = u_id WHERE op2_id = %d;", user_data[arg->idThread]);

    rc = sqlite3_prepare_v2(champ_db, select, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot exec statement: %s\n", sqlite3_errmsg(champ_db));
        return;
    }

    while (1) {
        rc = sqlite3_step(stmt);

        if (rc != SQLITE_ROW) {
            break;
        }

        char m_id[256];
        sprintf(m_id, "%d", sqlite3_column_int(stmt, 0));
        write(fd, m_id, strlen(m_id));
        write(fd, "\n", 1);
        const char *opponent_name = sqlite3_column_text(stmt, 1);
        write(fd, opponent_name, strlen(opponent_name));
        write(fd, "\n", 1);
        const char *start_date = sqlite3_column_text(stmt, 2);
        write(fd, start_date, strlen(start_date));
        write(fd, "\n", 1);
    }

    sqlite3_finalize(stmt);

    return;
}

void show_history(void *arg, char *req, int fd, sqlite3 *champ_db) {
    printf("[SERVER]Showing history!\n");
    int response = 1;
    write(fd, &response, sizeof(int));

    return;
}

void postpone(void *arg, char *req, int fd, sqlite3 *champ_db) {
    printf("[SERVER]Postponing!\n");
    char *command = strtok(req, "_");
    char *m_id_string = strtok(NULL, "_");
    char *new_date = strtok(NULL, "_");
    int m_id = atoi(m_id_string);

    char update[256];
    memset(update, 0, 256);
    sprintf(update, "UPDATE Matches SET start_date = '%s' WHERE m_id = %d;", new_date, m_id);

    int rc = sqlite3_exec(champ_db, update, NULL, NULL, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot exec statement: %s\n", sqlite3_errmsg(champ_db));
        send_response(1, fd);
        return;
    }

    send_response(0, fd);
    return;
}

// Function to generate a unique ID
int generate_unique_u_id(sqlite3 *user_db) {
    char *poll = "SELECT * FROM Users WHERE u_id=?;";
    int user_exists = 0;
    char *errMsg = 0;

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(user_db, poll, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(user_db));
        return 0;
    }

    int id = 0;

    while (1) {
        id = rand() % 100000;

        rc = sqlite3_bind_int(stmt, 1, id);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(user_db));
            return 0;
        }
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW) {
            break;
        }
    }

    sqlite3_finalize(stmt);

    return id;
}

int generate_unique_t_id(sqlite3 *tournament_db) {
    char *poll = "SELECT * FROM Tournaments WHERE t_id=?;";
    int tournament_exists = 0;
    char *errMsg = 0;

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(tournament_db, poll, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(tournament_db));
        return 0;
    }

    int id = 0;

    while (1) {
        id = rand() % 100000;

        rc = sqlite3_bind_int(stmt, 1, id);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(tournament_db));
            return 0;
        }
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW) {
            break;
        }
    }

    sqlite3_finalize(stmt);

    return id;
}

int generate_unique_m_id(sqlite3 *match_db) {
    char *poll = "SELECT * FROM Matches WHERE m_id=?;";
    int match_exists = 0;
    char *errMsg = 0;

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(match_db, poll, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot prepare statement: %s\n", sqlite3_errmsg(match_db));
        return 0;
    }

    int id = 0;

    while (1) {
        id = rand() % 100000;

        rc = sqlite3_bind_int(stmt, 1, id);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Cannot bind parameter: %s\n", sqlite3_errmsg(match_db));
            return 0;
        }
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW) {
            break;
        }
    }

    sqlite3_finalize(stmt);

    return id;
}