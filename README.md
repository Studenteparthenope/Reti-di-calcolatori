# Reti-di-calcolatori
Il progetto descritto prevede l'implementazione di una comunicazione server-client in linguaggio C per la gestione di ricette culinarie. In particolare, il server espone delle funzionalità di autenticazione utente e di ricerca di ricette, mentre il client si connette al server per effettuare tali operazioni.
Il progetto sfrutta il protocollo TCP/IP per la comunicazione tra client e server, con l’uso di socket, ed è strutturato per supportare l’autenticazione e la ricerca di ricette tramite file di testo.
Il server e il client sono scritti in C e interagiscono con due file di database: uno contenente le credenziali degli utenti (DbAccount.txt) e uno contenente le ricette (ricette.txt). 
La comunicazione avviene attraverso il protocollo di rete TCP su una porta predefinita (8100).
Tale progetto è stato realizzato in collaborazione della collega Emanuela Baldassarre.
