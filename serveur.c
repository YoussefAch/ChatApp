/* version 0 (PM, 16/4/17) :
	Le serveur de conversation
	- crée un tube (fifo) d'écoute (avec un nom fixe : ./ecoute)
	- gère un maximum de maxParticipants conversations : 
		* accepter les demandes de connexion tube d'écoute) de nouveau(x) participant(s)
			 si possible
			-> initialiser et ouvrir les tubes de service (entrée/sortie) fournis 
				dans la demande de connexion
		* messages des tubes (fifo) de service en entrée 
			-> diffuser sur les tubes de service en sortie
	- détecte les déconnexions lors du select
	- se termine à la connexion d'un client de pseudo "fin"
	Protocole
	- suppose que les clients ont créé les tube d'entrée/sortie avant
		la demande de connexion, nommés par le nom du client, suffixés par _C2S/_S2C.
	- les échanges par les tubes se font par blocs de taille fixe, dans l'idée d'éviter
	  le mode non bloquant.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <stdbool.h>
#include <sys/stat.h>

#define NBPARTICIPANTS 5 	/* seuil au delà duquel la prise en compte de nouvelles
								 connexions sera différée */
#define TAILLE_MSG 128		/* nb caractères message complet (nom+texte) */
#define TAILLE_NOM 25		/* nombre de caractères d'un pseudo */
#define NBDESC FD_SETSIZE-1	/* pour un select éventuel
								 (macros non definies si >= FD_SETSIZE) */
#define TAILLE_TUBE 512		/*capacité d'un tube */

typedef struct ptp {
    bool actif;
    char nom [TAILLE_NOM];
    int in;		/* tube d'entrée */
    int out;	/* tube de sortie */
} participant;

typedef struct dde {
    char nom [TAILLE_NOM];
} demande;

static const int maxParticipants = NBPARTICIPANTS+1+TAILLE_TUBE/sizeof(demande);

participant participants [NBPARTICIPANTS+1+TAILLE_TUBE/sizeof(demande)]; //maxParticipants
char buf[TAILLE_TUBE];
int nbactifs = 0;

/* Initialiser */
void effacer(int i) {
    participants[i].actif = false;
    bzero(participants[i].nom, TAILLE_NOM*sizeof(char));
    participants[i].in = -1;
    participants[i].out = -1;
}

void diffuser(char *dep) {
/* à faire */
	int i;
	for (i = 0; i < maxParticipants ; i++) {
			if (participants[i].actif) {
				if ( (write(participants[i].out,dep,TAILLE_MSG)) <= 0) {
					perror("erreur diffusion");
					exit(1);
				}
			}
	}
}


void desactiver (int p) {
/* traitement d'un participant déconnecté (à faire) */
	participants[p].actif = false;
	close(participants[p].in);
	close(participants[p].out);
}


void ajouter(char *dep) {
/*  Pré : nbactifs < maxParticpants
	
	Ajoute un nouveau participant de pseudo dep.
	Si le participant est "fin", termine le serveur.
	
	(à faire)
*/

	/** Chercher une place libre */
	int i = 0;
	while (i < maxParticipants && participants[i].actif) {
		i++;
	}
	
	/** Ajouter le participant dans cette place */
	participants[i].actif = true;
	strcpy(participants[i].nom,dep);
	
	
	char C2S[TAILLE_NOM + 5];
	char S2C[TAILLE_NOM + 5];

	
	strcpy(C2S, dep);
	strcat(C2S, "_C2S");
	
	strcpy(S2C, dep);
	strcat(S2C, "_S2C");	
	
	
	
	participants[i].in = open(C2S, O_RDONLY);
	participants[i].out = open(S2C, O_WRONLY);
	
	fcntl(participants[i].in, F_SETFL, fcntl(participants[i].in, F_GETFL) | O_NONBLOCK);
	fcntl(participants[i].out, F_SETFL, fcntl(participants[i].out, F_GETFL) | O_NONBLOCK);

}

int main (int argc, char *argv[]) {
	
    int i,nlus,necrits,res;
    int ecoute;		/* descripteur d'écoute */
    fd_set readfds; /* ensemble de descripteurs écoutés par un select éventuel */
    char * buf0;   /* pour parcourir le contenu d'un tube, si besoin */

	buf0 = (char *)malloc(TAILLE_TUBE);
	
	char  service[TAILLE_MSG]; /* message du service */
	
	char msgAuRevoir[TAILLE_MSG]; /* Message au revoir du client pour terminer la session client */
	
	
	
	
    /* création (puis ouverture) du tube d'écoute */
    mkfifo("./ecoute",S_IRUSR|S_IWUSR); // mmnémoniques sys/stat.h: S_IRUSR|S_IWUSR = 0600
    ecoute=open("./ecoute",O_RDONLY);

    for (i=0; i<maxParticipants; i++) {
        effacer(i);
    }
    

    

    while (true) {
        printf("participants actifs : %d\n",nbactifs);
		/* boucle du serveur : traiter les requêtes en attente 
			sur le tube d'écoute et les tubes d'entrée
			
			(à compléter)
		*/
		int i;
		
		FD_ZERO(&readfds);
		FD_SET(ecoute, &readfds);
		
		for (i=0; i<maxParticipants; i++) {
			if (participants[i].actif) {
				FD_SET(participants[i].in, &readfds);
				
				
			}
		} 
		
		select(NBDESC, &readfds, NULL, NULL, NULL);
		
		
		

		/** Vérification du tube d'écoute */
		if (FD_ISSET(ecoute, &readfds)) {
			
			/** lecture du contenu du tube d'écoute */
			
			/** Vider le buffer / initialisation */
			bzero(buf0, TAILLE_TUBE);
			
			/** Vérifier le tube d'écoute */
			if ( (nlus = read(ecoute, buf0, TAILLE_TUBE)) > 0 ) {
				buf0[nlus] = '\0';
				
				if (nbactifs < NBPARTICIPANTS) {
					/** ajout du client */
					ajouter(buf0);
					
					/** Préparation du message service */
					strcpy(service,"[service] ");
					strcat(service,buf0);
					strcat(service," a rejoint la conversation");
					
					/** Diffusion du message */
					diffuser(service);
					
					/** Mise à jour du nombre de clients actifs */
					nbactifs++;
				} else {
					printf("attendez qu'un utilisateur quitte la conversation\n");
				}
				
			}else {
				
				
				break;
			}	
		}
		
		
		/** Vérification des tubes C2S */
		for (i=0; i<maxParticipants; i++) {
			
			if ( (participants[i].actif) && (FD_ISSET(participants[i].in, &readfds)) ) {
				
				/** Vider le buffer */
				bzero(buf0, TAILLE_MSG);
				
				
				if ( (necrits = read(participants[i].in, buf0, TAILLE_MSG)) > 0 ) {
					
					
					buf0[necrits] = '\0';
					
					/** Diffusion du message */
					diffuser(buf0);
					
					/** Construction du message au revoir à l'aide du nom du client */
					strcpy(msgAuRevoir, "[");
					strcat(msgAuRevoir, participants[i].nom);
					strcat(msgAuRevoir, "] ");
					strcat(msgAuRevoir, "au revoir");
					
					/** Traitement de la déconnexion */
					if (strcmp(buf0 , msgAuRevoir) == 0) {
						
						/** Construction du message à diffuser lors de la déconnexion */
						strcpy(service,"[service] ");
						strcat(service, participants[i].nom);
						strcat(service, " a quitté la conversation");
						
						/** diffusion du message */
						diffuser(service);
						
						/** libérer la place du client déconnecté */
						desactiver(i);
						
						/** Mise à jour du nombre actifs de clients */
						nbactifs--;
					}
				} else {
					
					
					break;
				}
				
			}
		}		

		
		
    }
    unlink("./ecoute");
    printf("Fin serveur\n");
}
