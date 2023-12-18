#include "terminal_utils.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define PORT 2024

int logged_flag = 0;
int fd_server;

void log_prompt();
void command_prompt();
void login_account();
void register_account();
void create_tournament();
void join_tournament();
void show_tournaments();
void show_matches();
void show_history();
void postpone();
void exit_application();

int send_fmt_log(char *username, char *password) {
    char req_fmt[1000];
    if (strcmp(username, "") != 0 && strcmp(password, "") != 0) {
        sprintf(req_fmt, "VAL_L_%s_%s\n", username, password);

        if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
            perror("Server: Error at writing to specified fd.");
            exit(1);
        }

        return 0;
    }
    return -1;
}

int send_fmt_reg(char *username, char *password, char *email) {
    char req_fmt[1000];
    if (strcmp(username, "") != 0 && strcmp(password, "") != 0 && strcmp(email, "") != 0) {
        sprintf(req_fmt, "VAL_R_%s_%s_%s\n", username, password, email);

        if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
            perror("[CLIENT] Error at writing to specified fd.");
            exit(1);
        }

        return 0;
    }
    return 1;
}

int send_fmt_crt(char *tournament_name, char *tournament_date, char *tournament_game, char *tournament_no_players, char *choice_string) {
    char req_fmt[1000];
    if (strcmp(tournament_name, "") != 0 && strcmp(tournament_date, "") != 0 && strcmp(tournament_game, "") != 0 && strcmp(tournament_no_players, "") != 0 && strcmp(choice_string, "") != 0) {
        sprintf(req_fmt, "CRT_%s_%s_%s_%s_%s\n", tournament_name, tournament_date, tournament_game, tournament_no_players, choice_string);

        if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
            perror("Server: Error at writing to specified fd.");
            exit(1);
        }

        return 0;
    }
    return -1;
}

int send_fmt_jnt(char *tournament_name) {
    char req_fmt[1000];
    if (strcmp(tournament_name, "") != 0) {
        sprintf(req_fmt, "JNT_%s\n", tournament_name);

        if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
            perror("Server: Error at writing to specified fd.");
            exit(1);
        }

        return 0;
    }
    return -1;
}

int send_fmt_sht() {
    char req_fmt[1000];
    sprintf(req_fmt, "SHT\n");

    if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
        perror("Server: Error at writing to specified fd.");
        exit(1);
    }

    return 0;
}

int send_fmt_shm() {
    char req_fmt[1000];
    sprintf(req_fmt, "SHM\n");

    if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
        perror("Server: Error at writing to specified fd.");
        exit(1);
    }

    return 0;
}

int send_fmt_shh() {
    char req_fmt[1000];
    sprintf(req_fmt, "SHH\n");

    if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
        perror("Server: Error at writing to specified fd.");
        exit(1);
    }

    return 0;
}

int send_fmt_pst(int match_id, char *pst_date) {
    char req_fmt[1000];
    sprintf(req_fmt, "PST_%d_%s\n", match_id, pst_date);

    if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
        perror("Server: Error at writing to specified fd.");
        exit(1);
    }

    return 0;
}

void register_account() {
    char username[256];
    char password[256];
    char email[256];
    char r_password[256];
    int response;

    printf("Username: ");
    fflush(stdout);
    scanf("%[^\n]%*c", username);

    printf("Email address: ");
    fflush(stdout);
    scanf("%[^\n]%*c", email);
    terminal_alter();
    printf("Password: ");
    fflush(stdout);
    star_get(password, 256);
    printf("\n");
    terminal_unalter();
    terminal_alter();
    printf("Repeat password: ");
    fflush(stdout);
    star_get(r_password, 256);
    printf("\n");
    terminal_unalter();

    while (strcmp(password, r_password)) {
        printf("The passwords do not match! Please enter them again.\n");
        memset(password, 0, 256);
        memset(r_password, 0, 256);
        terminal_alter();
        printf("Password: ");
        fflush(stdout);
        star_get(password, 256);
        printf("\n");
        terminal_unalter();
        terminal_alter();
        printf("Repeat password: ");
        fflush(stdout);
        star_get(r_password, 256);
        printf("\n");
        terminal_unalter();
    }

    if (send_fmt_reg(username, password, email) != 0) {
        perror("Error at sending the request to server. Invalid data.\n");
        exit(1);
    }

    if (read(fd_server, &response, sizeof(response)) < 0) {
        perror("Error at receiving the response from server.\n");
        exit(1);
    }

    if (response == 1) {
        printf("Username already exists!\n");
        log_prompt();
    } else if (response == 2) {
        printf("Email address already exists!\n");
        log_prompt();
    } else {
        logged_flag = 0;
        printf("Succesful register!\nPlease log in. \n");
    }

    login_account();
}

void login_account() {
    char username[256];
    char password[256];
    char choice_string[256];
    int choice;
    int response = 0;

    while (!response) {
        printf("Username: ");
        fflush(stdout);
        scanf("%[^\n]%*c", username);
        terminal_alter();
        printf("Password: ");
        fflush(stdout);
        star_get(password, 256);
        printf("\n");
        terminal_unalter();

        if (send_fmt_log(username, password) != 0) {
            perror("Error at sending the request to server. Invalid data.\n");
            exit(1);
        }

        if (read(fd_server, &response, sizeof(response)) < 0) {
            perror("Error at receiving the response from server.\n");
            exit(1);
        }

        if (response == 1) {
            printf("\nIncorrect username or password!\n 1 - Try again \n 2 - Go back \n");
            fflush(stdout);
            scanf("%[^\n]%*c", choice_string);
            choice = atoi(choice_string);
            switch (choice) {
            case 1:
                break;
            case 2:
                log_prompt();
                break;
            default:
                printf("Invalid choice. \n");
                break;
            }
        } else {
            logged_flag = 1;
            printf("Succesful login!\n");
            command_prompt();
            break;
        }
    }
}

void create_tournament() {
    char tournament_name[256];
    char tournament_date[256];
    char tournament_game[256];
    char tournament_no_players[256];
    char choice_string[256];
    int response = 0;

    printf("Please provide the details for the new tournament:\n");
    printf("Tournament name: ");
    scanf("%[^\n]%*c", tournament_name);
    printf("Tournament date (in the format 'YYYY-MM-DD HH:MM'): ");
    scanf("%[^\n]%*c", tournament_date);
    printf("Tournament's game: ");
    scanf("%[^\n]%*c", tournament_game);
    printf("Tournament's number of players (the number must be a power of 2!): ");
    scanf("%[^\n]%*c", tournament_no_players);
    printf("Tournament's type: \n 1 - Single elimination -> Once a participant looses, he is out of the tournament! \n 2 - Double elimination -> Participants can loose a match once.\n");
    scanf("%[^\n]%*c", choice_string);

    if (strcmp(choice_string, "1") != 0 && strcmp(choice_string, "2") != 0) {
        printf("Invalid choice. \n");
        return;
    }

    send_fmt_crt(tournament_name, tournament_date, tournament_game, tournament_no_players, choice_string);

    if (read(fd_server, &response, sizeof(response)) < 0) {
        perror("Error at receiving the response from server.\n");
        exit(1);
    }

    if (response == 0)
        printf("Tournament created successfully! \n");
    else if (response == 1)
        printf("Tournament creation failed! Tournament name already used!\n");
}

void join_tournament() {
    char tournament_name[256];
    char choice_string[256];
    int response = 0;

    printf("Please enter the name of the tournament you want to join: ");
    scanf("%[^\n]%*c", tournament_name);

    send_fmt_jnt(tournament_name);

    if (read(fd_server, &response, sizeof(response)) < 0) {
        perror("[CLIENT] Error at receiving the response from server.\n");
        exit(1);
    }

    if (response == 1) {
        printf("Tournament does not exist! \n");
        return;
    } else if (response == 2) {
        printf("You have already joined this tournament! \n");
        return;
    } else if (response == 3) {
        printf("Tournament is full! \n");
        return;
    }

    printf("Tournament joined successfully! \n");
}

void show_tournaments() {
    int response = 0;
    send_fmt_sht();

    if (read(fd_server, &response, sizeof(response)) < 0) {
        perror("[CLIENT] Error at receiving the response from server.\n");
        exit(1);
    }

    if (response == 1) {
        printf("Currently there are no available tournaments. All are full! \n");
        return;
    }

    int no_tournaments = 0;
    if (read(fd_server, &no_tournaments, sizeof(no_tournaments)) < 0) {
        perror("[CLIENT] Error at receiving the response from server.\n");
        exit(1);
    }

    printf("Currently there are %d available tournaments: \n", no_tournaments);

    char tournament_name[256], tournament_game[256], start_date[256], current_players[10], max_players[10];

    for (int i = 0; i < no_tournaments; i++) {
        int pos = 0;
        char c = 0;
        while (c != '\n') {
            if (read(fd_server, &c, 1) == -1) {
                perror("[CLIENt]Error at reading from specified client fd.\n");
                exit(1);
            }

            if (c != '\n')
                tournament_name[pos++] = c;
        }
        tournament_name[pos] = '\0';

        printf("%d. %s", i + 1, tournament_name);

        pos = 0;
        c = 0;

        while (c != '\n') {
            if (read(fd_server, &c, 1) == -1) {
                perror("[CLIENt]Error at reading from specified client fd.\n");
                exit(1);
            }

            if (c != '\n')
                tournament_game[pos++] = c;
        }

        tournament_game[pos] = '\0';

        printf("\n\tThe game is: %s", tournament_game);

        pos = 0;
        c = 0;

        while (c != '\n') {
            if (read(fd_server, &c, 1) == -1) {
                perror("[CLIENt]Error at reading from specified client fd.\n");
                exit(1);
            }

            if (c != '\n')
                start_date[pos++] = c;
        }

        start_date[pos] = '\0';

        printf("\n\tIt will start on: %s", start_date);

        pos = 0;
        c = 0;

        while (c != '\n') {
            if (read(fd_server, &c, 1) == -1) {
                perror("[CLIENt]Error at reading from specified client fd.\n");
                exit(1);
            }

            if (c != '\n')
                current_players[pos++] = c;
        }

        current_players[pos] = '\0';

        pos = 0;
        c = 0;

        while (c != '\n') {
            if (read(fd_server, &c, 1) == -1) {
                perror("[CLIENt]Error at reading from specified client fd.\n");
                exit(1);
            }

            if (c != '\n')
                max_players[pos++] = c;
        }

        max_players[pos] = '\0';

        printf("\n\tThere are currently %d players registered out of %d.\n", atoi(current_players), atoi(max_players));
    }
}

void show_matches() {
    int response = 0;
    send_fmt_shm();

    if (read(fd_server, &response, sizeof(response)) < 0) {
        perror("[CLIENT] Error at receiving the response from server.\n");
        exit(1);
    }

    if (response == 1) {
        printf("You have no upcoming matches! \n");
        return;
    } else {
        if (read(fd_server, &response, sizeof(response)) < 0) {
            perror("[CLIENT] Error at receiving the response from server.\n");
            exit(1);
        }
        printf("You have %d upcoming matches: \n", response);
    }

    char m_id[256], op_name[256], m_date[256];

    for (int i = 0; i < response; i++) {
        int pos = 0;
        char c = 0;

        while (c != '\n') {
            if (read(fd_server, &c, 1) == -1) {
                perror("[CLIENt]Error at reading from specified client fd.\n");
                exit(1);
            }

            if (c != '\n')
                m_id[pos++] = c;
        }

        m_id[pos] = '\0';

        pos = 0;
        c = 0;

        while (c != '\n') {
            if (read(fd_server, &c, 1) == -1) {
                perror("[CLIENt]Error at reading from specified client fd.\n");
                exit(1);
            }

            if (c != '\n')
                op_name[pos++] = c;
        }

        op_name[pos] = '\0';

        pos = 0;
        c = 0;

        while (c != '\n') {
            if (read(fd_server, &c, 1) == -1) {
                perror("[CLIENt]Error at reading from specified client fd.\n");
                exit(1);
            }

            if (c != '\n')
                m_date[pos++] = c;
        }

        m_date[pos] = '\0';

        printf("%d. You will play against %s on %s --- Match id: %d\n", i + 1, op_name, m_date, atoi(m_id));
    }
}

void show_history() {
    int response = send_fmt_shh();
}

void postpone() {
    int response = 0;
    char match_id[256], pst_date[256];

    printf("Please enter the id of the match you want to postpone: ");
    scanf("%[^\n]%*c", match_id);
    printf("Please enter the new date of the match (in the format 'YYYY-MM-DD HH:MM'): ");
    scanf("%[^\n]%*c", pst_date);

    send_fmt_pst(atoi(match_id), pst_date);

    if (read(fd_server, &response, sizeof(response)) < 0) {
        perror("[CLIENT] Error at receiving the response from server.\n");
        exit(1);
    }

    if (response == 0) {
        printf("Match with id %d was postponed! \n", atoi(match_id));
        return;
    } else {
        printf("Something went wrong! \n");
        return;
    }
}

void exit_application() {
    const char *req_fmt = "EXT\n";

    if (write(fd_server, req_fmt, strlen(req_fmt)) == -1) {
        perror("Server: Error at writing to specified fd.");
        exit(1);
    }

    close(fd_server);
    exit(0);
}

void log_prompt() {
    int choice;
    char choice_string[256];

    printf("\nPlease register a new account or log in into an existing account. \n 1 - Register \n 2 - Log in \n 3 - Exit the application \n");
    scanf("%[^\n]%*c", choice_string);
    choice = atoi(choice_string);

    switch (choice) {
    case 1:
        printf("Registering new account... \n");
        register_account();
        break;
    case 2:
        printf("Logging in... \n");
        login_account();
        break;
    case 3:
        printf("Exiting the application... \n");
        exit_application();
        break;
    default:
        printf("Invalid choice. \n");
        break;
    }
}

void command_prompt() {
    int choice;
    char choice_string[256];

    printf("\nPlease choose one of the following actions. \n 1 - Create Tournament \n 2 - Join Tournament \n 3 - Show available tournaments \n 4 - Show my matches \n 5 - Show history of my tournaments \n 6 - Postpone one of my matches \n 7 - Exit the application \n");
    scanf("%[^\n]%*c", choice_string);
    choice = atoi(choice_string);

    switch (choice) {
    case 1:
        printf("Creating new tournament... \n");
        create_tournament();
        break;
    case 2:
        printf("Joining tournament... \n");
        join_tournament();
        break;
    case 3:
        printf("Showing available tournaments... \n");
        show_tournaments();
        break;
    case 4:
        printf("Showing future matches... \n");
        show_matches();
        break;
    case 5:
        printf("Showing history of one tournament... \n");
        show_history();
        break;
    case 6:
        printf("Postpone match... \n");
        postpone();
        break;
    case 7:
        printf("Exiting the application... \n");
        exit_application();
        break;
    default:
        printf("Invalid choice. \n");
        break;
    }
}

int connect_to_server(char *server_ip) {
    int sd;
    struct sockaddr_in server;

    int nr = 0;
    char buf[10];

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_port = htons(PORT);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    return sd;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("The syntax for starting the client is: './<client_executable> <server_ip>'\n");
        return -1;
    }

    fd_server = connect_to_server(argv[1]);
    welcome_prompt();
    while (!logged_flag)
        log_prompt();
    while (logged_flag)
        command_prompt();

    close(fd_server);
}
