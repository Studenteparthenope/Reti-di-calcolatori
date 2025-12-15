#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define SERVER_PORT 8100
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 256

// Funzioni dichiarate
int authenticate_user(int sock);  // Funzione per autenticare l'utente
int search_recipe(int sock);      // Funzione per cercare una ricetta

int main() {
    int client_sock;
    struct sockaddr_in server_addr;

    // Creazione del socket per il client
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1) {
        perror("Errore nella creazione della socket");
        exit(1);
    }

    // Impostazione dell'indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connessione al server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nella connessione al server");
        close(client_sock);
        exit(1);
    }

    // Autenticazione dell'utente
    if (!authenticate_user(client_sock)) {
        printf("Autenticazione fallita.\n");
        close(client_sock);
        exit(1);
    }

    // Ricerca della ricetta
    if (!search_recipe(client_sock)) {
        printf("Ricetta non trovata.\n");
        close(client_sock);
        exit(1);
    }

    close(client_sock);  // Chiude il socket del client
    return 0;
}

// Funzione per autenticare l'utente
int authenticate_user(int sock) {
    char username[50], password[50];
    int response;

    // Richiesta e inserimento del nome utente
    printf("Inserisci il nome utente: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';  // Rimuove il carattere di fine riga

    // Richiesta e inserimento della password
    printf("Inserisci la password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';  // Rimuove il carattere di fine riga

    // Invio delle credenziali al server
    if (send(sock, username, sizeof(username), 0) == -1 || send(sock, password, sizeof(password), 0) == -1) {
        perror("Errore nell'invio delle credenziali");
        return 0;
    }

    // Riceve la risposta dal server per l'autenticazione
    if (recv(sock, &response, sizeof(response), 0) == -1) {
        perror("Errore nel ricevere la risposta di autenticazione");
        return 0;
    }

    // Se la risposta è 1, l'autenticazione è riuscita
    return response == 1;
}

// Funzione per cercare una ricetta
int search_recipe(int sock) {
    char recipe_name[100], procedure[500], preparation_time[50], difficulty[50];
    int response;

    // Elenco delle ricette disponibili nel database
    printf("Le ricette nel presente database sono:Spaghettata, Pollo al limone, Hummus di ceci\n");
    printf("Involtini di prosciutto crudo con formaggio cremoso, Palline al cacao e al cocco, Pasta con zucchine e parmigiano\n");

    // Richiesta del nome della ricetta da cercare
    printf("Inserisci il nome della ricetta: ");
    fgets(recipe_name, sizeof(recipe_name), stdin);
    recipe_name[strcspn(recipe_name, "\n")] = '\0';  // Rimuove il carattere di fine riga

    // Invio del nome della ricetta al server
    if (send(sock, recipe_name, sizeof(recipe_name), 0) == -1) {
        perror("Errore nell'invio del nome della ricetta");
        return 0;
    }

    // Riceve la risposta dal server riguardo la ricetta
    if (recv(sock, &response, sizeof(response), 0) == -1) {
        perror("Errore nel ricevere la risposta sulla ricetta");
        return 0;
    }

    // Se la ricetta è trovata (response == 1), riceve i dettagli
    if (response == 1) {
        // Riceve la procedura, il tempo di preparazione e la difficoltà
        if (recv(sock, procedure, sizeof(procedure), 0) == -1 ||
            recv(sock, preparation_time, sizeof(preparation_time), 0) == -1 ||
            recv(sock, difficulty, sizeof(difficulty), 0) == -1) {
            perror("Errore nel ricevere i dettagli della ricetta");
            return 0;
        }

        // Stampa i dettagli della ricetta trovata
        printf("\nRicetta trovata:\n");
        printf("Procedura: %s\n", procedure);
        printf("Tempo di preparazione: %s\n", preparation_time);
        printf("Difficoltà: %s\n", difficulty);
    }

    return response == 1;  // Restituisce 1 se la ricetta è stata trovata, altrimenti 0
}
