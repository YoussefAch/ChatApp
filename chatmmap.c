/* version 0.1 (PM, 23/4/17) :
	La discussion est un tableau de messages, couplé en mémoire partagée.
	Un message comporte un auteur, un texte et un numéro d'ordre (croissant).
	Le numéro d'ordre permet à chaque participant de détecter si la discussion a évolué
	depuis la dernière fois qu'il l'a affichée. 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h> /* définit mmap  */
#include <signal.h>

#define TAILLE_AUTEUR 25
#define TAILLE_TEXTE 128
#define NB_LIGNES 20

/* message : numéro d'ordre, auteur (25 caractères max), texte (128 caractères max) */
struct message {
  int numero;
  char auteur [TAILLE_AUTEUR];
  char texte [TAILLE_TEXTE];
};

/* discussion (20 derniers messages) */
struct message * discussion;

/* dernier message reçu */
int dernier0 = 0 ;
int num = 0;
int dernierAff = 1;
/* afficher la discussion */
void afficher() {
	int i;
	for (i=0; i<NB_LIGNES; i++) {
		printf("[%s] : %s\n", discussion[i].auteur, discussion[i].texte);
	}
	printf("=========================================================================\n");
}

/* traitant : rafraichir la discussion, s'il y a lieu */
void traitant_sigAlarm (int sig) {
	/* à faire */

	
	/**
		A chaque fois que l'on ajoute un nouveau message, on lui donne un numéro égal à celui du dernier +1.
		De cette manière, on sait que le dernier message a changé si son numéro a évolué,
		cad est différent de dernier0
	*/
	
	if (num != dernier0) {
		afficher();
		dernier0++;
		
	}
	alarm(4);

}

int main (int argc, char *argv[]) { 
	struct message m;
	int i,taille,fdisc;
	int nlus;
 	char qq [1];
 	FILE * fdf;
	
	if (argc != 3) {
		printf("usage: %s <discussion> <participant>\n", argv[0]);
		exit(1);
	}

	 /* ouvrir et coupler discussion */
	if ((fdisc = open (argv[1], O_RDWR | O_CREAT, 0666)) == -1) {
		printf("erreur ouverture discussion\n");
		exit(2);
	}
	
	/*	mmap ne spécifie pas quel est le resultat d'une ecriture *apres* la fin d'un 
		fichier couple (SIGBUS est une possibilite, frequente). Il faut donc fixer la 
		taille du fichier destination à la taille du fichier source *avant* le couplage. 
		On utilise ici lseek (a la taille du fichier source) + write d'un octet, 
		qui sont deja connus.
	*/
	

	signal(SIGALRM, traitant_sigAlarm);
	
	qq[0]='x';
	taille = sizeof(struct message)*NB_LIGNES;
 	lseek (fdisc, taille, SEEK_SET);
 	write (fdisc, qq, 1);
 	char buf[TAILLE_TEXTE];
 	
	discussion = mmap(0, taille, PROT_READ | PROT_WRITE, MAP_SHARED, fdisc, 0);
	
 	
 	/* à compléter : saisie des messages, gestion de la discussion*/
 	while(1) {
		
		
		
		/* DEBUT Construction du message ----------------------------------------------*/

			/** Je vide le buffer */
			bzero(buf, TAILLE_TEXTE);
			
			/** Lire un texte de messages sur l'entree standard */
			if ( (nlus = read(0, buf,TAILLE_TEXTE)) < 0 ) {
				perror("Lecture echoue");
			} 
			buf[nlus-1] = '\0';
			
			
			/** remplir l'auteur */
			strcpy(m.auteur, argv[2]);
			
			/** remplir numero */
			m.numero = num++;
			
			
			/** remplir le message */
			strcpy(m.texte, buf);

		/* FIN Construction du message ----------------------------------------------*/	
		
		
		
		
		
		/* décale tout d'une case vers le haut-------------------------------------- */
		memmove(discussion, discussion + 1, sizeof(struct message)*(NB_LIGNES-1));
	
		
		
		/* mettre le message dans le trou -------------------------------------------*/
		discussion[19] = m;
		
		
		
		/* Lancer une alarme */
		alarm(1);
		
		
	}
 	
 	
 	
	close(fdisc);
 	exit(0);
}
