#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define SERVER_PORT 8100
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 256
#define DB_ACCOUNT_FILE "DbAccount.txt"
#define DB_RECIPES_FILE "ricette.txt"

// Funzioni dichiarate
int authenticate_user(int sock);  // Funzione per autenticare un utente
int search_recipe(int sock);      // Funzione per cercare una ricetta
int check_credentials(const char *username, const char *password);  // Verifica le credenziali dell'utente
int find_recipe(const char *recipe_name, char *procedure, char *preparation_time, char *difficulty);  // Cerca la ricetta nel DB
void *client_handler(void *arg);  // Gestisce la comunicazione con un client

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Creazione del socket per il server
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Errore nella creazione del socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Impostazione delle opzioni del socket per permettere il riutilizzo dell'indirizzo
    int reuse = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("Errore nell'impostare SO_REUSEADDR");
        close(server_sock);
        exit(1);
    }

    // Associa il socket all'indirizzo e alla porta
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nell'associazione del socket");
        close(server_sock);
        exit(1);
    }

    // Inizia ad ascoltare le connessioni in arrivo
    if (listen(server_sock, 5) == -1) {
        perror("Errore nell'ascolto delle connessioni");
        close(server_sock);
        exit(1);
    }

    printf("Server in ascolto sulla porta %d...\n", SERVER_PORT);

    while (1) {
        // Accetta una connessione in arrivo
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1) {
            perror("Errore nell'accettare la connessione");
            continue;
        }

        printf("Connessione stabilita con un nuovo client\n");

        // Crea un thread per gestire il client connesso
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, (void *)&client_sock) != 0) {
            perror("Errore nella creazione del thread");
            close(client_sock);
        } else {
            pthread_detach(tid);  // Distacca il thread per evitare blocchi
        }
    }

    close(server_sock);  // Chiude il socket del server
    return 0;
}

// Funzione che gestisce le richieste di un singolo client
void *client_handler(void *arg) {
    int client_sock = *((int *)arg);

    // Autenticazione dell'utente
    if (!authenticate_user(client_sock)) {
        printf("Autenticazione fallita.\n");
        close(client_sock);
        return NULL;
    }

    // Ricerca della ricetta richiesta
    if (!search_recipe(client_sock)) {
        printf("Ricetta non trovata.\n");
        close(client_sock);
        return NULL;
    }

    close(client_sock);  // Chiude la connessione con il client
    return NULL;
}

// Funzione per autenticare l'utente
int authenticate_user(int sock) {
    char username[50], password[50];
    int response;

    // Riceve il nome utente e la password dal client
    if (recv(sock, username, sizeof(username), 0) == -1 || recv(sock, password, sizeof(password), 0) == -1) {
        perror("Errore nel ricevere le credenziali");
        return 0;
    }

    // Verifica le credenziali
    if (check_credentials(username, password)) {
        response = 1;  // Autenticazione riuscita
    } else {
        response = 0;  // Autenticazione fallita
    }

    // Invia la risposta al client
    if (send(sock, &response, sizeof(response), 0) == -1) {
        perror("Errore nell'invio della risposta di autenticazione");
        return 0;
    }

    return response == 1;  // Restituisce 1 se l'autenticazione è riuscita, altrimenti 0
}

// Funzione che controlla le credenziali nel file di account
int check_credentials(const char *username, const char *password) {
    FILE *file = fopen(DB_ACCOUNT_FILE, "r");
    if (file == NULL) {
        perror("Errore nell'aprire il file degli account");
        return 0;
    }

    char file_username[50], file_password[50];
    char line[BUFFER_SIZE];
    int found_username = 0;

    // Legge il file riga per riga
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Nome: ", 6) == 0) {
            sscanf(line, "Nome: %s", file_username);  // Estrae il nome utente
            found_username = 1;
        } else if (strncmp(line, "Password: ", 10) == 0 && found_username) {
            sscanf(line, "Password: %s", file_password);  // Estrae la password

            // Confronta le credenziali
            if (strcmp(file_username, username) == 0 && strcmp(file_password, password) == 0) {
                fclose(file);
                return 1;  // Credenziali corrette
            }
            found_username = 0;
        }
    }

    fclose(file);  // Chiude il file
    return 0;  // Credenziali errate
}

// Funzione per cercare una ricetta nel database
int search_recipe(int sock) {
    char recipe_name[100], procedure[500], preparation_time[50], difficulty[50];
    int response;

    // Riceve il nome della ricetta dal client
    if (recv(sock, recipe_name, sizeof(recipe_name), 0) == -1) {
        perror("Errore nel ricevere il nome della ricetta");
        return 0;
    }

    // Cerca la ricetta nel database
    if (find_recipe(recipe_name, procedure, preparation_time, difficulty)) {
        response = 1;  // Ricetta trovata
    } else {
        response = 0;  // Ricetta non trovata
    }

    // Invia la risposta al client
    if (send(sock, &response, sizeof(response), 0) == -1) {
        perror("Errore nell'invio della risposta sulla ricetta");
        return 0;
    }

    // Se la ricetta è stata trovata, invia i dettagli
    if (response == 1) {
        if (send(sock, procedure, sizeof(procedure), 0) == -1 ||
            send(sock, preparation_time, sizeof(preparation_time), 0) == -1 ||
            send(sock, difficulty, sizeof(difficulty), 0) == -1) {
            perror("Errore nell'invio dei dettagli della ricetta");
            return 0;
        }
    }

    return response == 1;  // Restituisce 1 se la ricetta è stata trovata, altrimenti 0
}

// Funzione per trovare una ricetta nel file
int find_recipe(const char *recipe_name, char *procedure, char *preparation_time, char *difficulty) {
    FILE *file = fopen(DB_RECIPES_FILE, "r");
    if (file == NULL) {
        perror("Errore nell'aprire il file delle ricette");
        return 0;
    }

    char file_recipe_name[100], line[BUFFER_SIZE];
    int recipe_found = 0;

    // Legge il file delle ricette riga per riga
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Ricetta: ", 9) == 0) {
            sscanf(line, "Ricetta: %s", file_recipe_name);  // Estrae il nome della ricetta

            // Se la ricetta è trovata, leggi i dettagli
            if (strcmp(file_recipe_name, recipe_name) == 0) {
                fgets(line, sizeof(line), file);  // Procedura
                sscanf(line, "Procedura: %[^\n]", procedure);

                fgets(line, sizeof(line), file);  // Tempo di preparazione
                sscanf(line, "Tempo di preparazione: %[^\n]", preparation_time);

                fgets(line, sizeof(line), file);  // Difficoltà
                sscanf(line, "Difficoltà: %[^\n]", difficulty);

                fclose(file);
                return 1;  // Ricetta trovata
            }
        }
    }

    fclose(file);  // Chiude il file
    return 0;  // Ricetta non trovata
}
