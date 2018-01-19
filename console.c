/* version 0 (PM, 16/4/17) :
	Le client de conversation
	- crée deux tubes (fifo) d'E/S, nommés par le nom du client, suffixés par _C2S/_S2C
	- demande sa connexion via le tube d'écoute du serveur (nom supposé connu),
		 en fournissant le pseudo choisi (max TAILLE_NOM caractères)
	- attend la réponse du serveur sur son tube _C2S
	- effectue une boucle : lecture sur clavier/S2C.
	- sort de la boucle si la saisie au clavier est "au revoir"
	Protocole
	- les échanges par les tubes se font par blocs de taille fixe TAILLE_MSG,
	- le texte émis via C2S est préfixé par "[pseudo] ", et tronqué à TAILLE_MSG caractères
Notes :
	-le  client de pseudo "fin" n'entre pas dans la boucle : il permet juste d'arrêter 
		proprement la conversation.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TAILLE_MSG 128		/* nb caractères message complet (nom+texte) */
#define TAILLE_NOM 25		/* nombre de caractères d'un pseudo */
#define NBDESC FD_SETSIZE-1  /* pour le select (macros non definies si >= FD_SETSIZE) */
#define TAILLE_TUBE 512		/*capacité d'un tube */
#define NB_LIGNES 20
#define TAILLE_SAISIE 1024

char pseudo [TAILLE_NOM];
char buf [TAILLE_TUBE];
char discussion [NB_LIGNES] [TAILLE_MSG]; /* derniers messages reçus */

void afficher(int depart) {
    int i;
    for (i=1; i<=NB_LIGNES; i++) {
        printf("%s\n", discussion[(depart+i)%NB_LIGNES]);
    }
    printf("=========================================================================\n");
}

int main (int argc, char *argv[]) {
	
    int i,nlus,necrits;
    char * buf0; 					/* pour parcourir le contenu d'un tube */

    int ecoute, S2C, C2S;			/* descripteurs tubes */
    int curseur = 0;				/* position dernière ligne reçue */

    fd_set readfds; 				/* ensemble de descripteurs écoutés par le select */

    char tubeC2S [TAILLE_NOM+5];	/* pour le nom du tube C2S */
    char tubeS2C [TAILLE_NOM+5];	/* pour le nom du tube S2C */
    char pseudo [TAILLE_NOM];
    char message [TAILLE_MSG];
    char saisie [TAILLE_SAISIE];

    if (!(argc == 2) || (strlen(argv[1]) > TAILLE_NOM*sizeof(char))) {
        printf("utilisation : %s <pseudo>\n", argv[0]);
        printf("Le pseudo ne doit pas dépasser 25 caractères\n");
        exit(1);
    }

    /** ouverture du tube d'écoute */
    ecoute = open("./ecoute",O_WRONLY);
    if (ecoute==-1) {
        printf("Le serveur doit être lance, et depuis le meme repertoire que le client\n");
        exit(2);
    }
    
    
    /** création des tubes de service  */
    
    /** Récupération du pseudo */
    strcpy(pseudo, argv[1]);
    
    /** Création du tube S2C */
    strcpy(tubeS2C, pseudo);
    strcat(tubeS2C, "_S2C");
	mkfifo(tubeS2C,S_IRUSR|S_IWUSR);
	
	/** Création du tube C2S */
    strcpy(tubeC2S, pseudo);
    strcat(tubeC2S, "_C2S");
	mkfifo(tubeC2S,S_IRUSR|S_IWUSR);
	
    /** connexion  */
	if ((write(ecoute, pseudo, TAILLE_NOM)) <= 0) {
		perror("erreur de connexion");
		exit(0);
	}
	C2S = open(tubeC2S, O_WRONLY);
	S2C = open(tubeS2C, O_RDONLY);
			

	
	
    if (strcmp(pseudo,"fin")!=0) {
    	/* client " normal " */
		/** initialisations  */
		memset (message, '\0', TAILLE_MSG);
		memset (saisie, '\0', TAILLE_SAISIE);
		buf0 = (char *)malloc(TAILLE_TUBE);

		
		/** Tant que le client n'a pas quitté la conversation */
        while (strcmp(saisie,"au revoir")!=0) {
        /* boucle principale (à faire) */
			FD_ZERO(&readfds);
			FD_SET(S2C, &readfds);
			FD_SET(0, &readfds);
			
			
	
			
			select(NBDESC, &readfds, NULL, NULL, NULL);
			
			/** On écoute sur le tube S2C */
			if (FD_ISSET(S2C, &readfds)) {
				
				/** Initialisation du buffer */
				memset(buf0, '\0', TAILLE_MSG);
				
				/** Lecture du contenu du tube */
				if ( (nlus = read(S2C, buf0, TAILLE_MSG)) > 0 ) {
					
					/** déplacer le curseur pour gérer l'affichage */
					curseur++;
					buf0[nlus] = '\0';
					
					/** ajouter le message à la discussion */
					strcpy(discussion[curseur], buf0);
					
					/** Affichage du message */
					afficher(curseur);
					
					
				} else {
					perror("On a pas pu lire le message qui est venu du serveur\n");
				}
				
			}
			
			
			/** On écoute l'entrée standard */
			if (FD_ISSET(0, &readfds)) {
				/** initialisations nécessaires */
				memset (message, '\0', TAILLE_MSG);
				memset (saisie, '\0', TAILLE_SAISIE);
				
				/** lecture de l'entrée standard */
				if ((nlus = read(0, saisie, TAILLE_SAISIE)) <= 0) {
					perror("Erreur lecture entrée standard\n");
				}
				saisie[nlus-1] = '\0';

				sprintf(message, "[%s] %s", pseudo, saisie);
				
				/** Ecriture du message dans le tube C2S */
				if ((write(C2S, message, TAILLE_MSG)) <= 0) {
					perror("erreur d'ecriture du message\n");
					exit(1);
				}
				
				
				
				
			}
			
        }
    }
    /* nettoyage (à faire) */
	
	unlink(tubeC2S);
	unlink(tubeS2C);
    printf("fin client\n");
    exit (0);
}
