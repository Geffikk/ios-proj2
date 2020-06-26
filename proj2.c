
 // - File:     rivercrossing.c
 // - Date:     27.4.2019
 // - Author:   Maros Geffert, xgeffe00@stud.fit.vutbr.cz
 // - Project:  IOS - Synchronizacia procesov - rivercrossing problem
 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

//Just DEFINES
#define CONDITION_OF_BOAT ((*pocet_hacker_lod >= 4) || (*pocet_serf_lod >= 4) || ((*pocet_hacker_lod >= 2 && *pocet_hacker_lod < 4) && (*pocet_serf_lod >= 2 && *pocet_serf_lod < 4 ))) //Condition for access on boat
#define CAPTAIN (1)
#define MEMBER (0) 
#define ERROR (-1)
#define CHYBA (1) 
#define CLOSE (1) 
#define OPEN (0) 
#define CHILD (0) 

//---------------- Global declarations ---------------------
//Flagy
int hack4=0; //aby som vedel kolkych serfov/hackerov mam odcitat od memberov na mole
int serf4=0; //aby som vedel kolkych serfov/hackerov mam odcitat od memberov na mole	
int serf_hack=0; //aby som vedel kolkych serfov/hackerov mam odcitat od memberov na mole

//File
FILE *output=NULL; //file, where im writing action of process

//Semafory
sem_t *sem_output; //semaphore, for critical spots
sem_t *mole_wait; //semafor, na korigovanie poctu prichadzajucich na lod
sem_t *hacker_sorted; //semafor, na sortovanie hackerov (kolkych pustim)
sem_t *serf_sorted; //semafor, na sortovanie serfov (kolkych pustim)
sem_t *captain_wait; //semafor, cakanie na kapitana
sem_t *waiting_for_landing; //semafor, ktory pusta dalsich hackerov/serfov az ked sa doplavia
sem_t *go_on_mole; //semafor, koriguje pocet ludi na mole

//Zdielane premenne
int *pocet_member_exit; //pocet memberov ktori odisli z lode
int *pocet_hacker_lod; //pocet hackerov ktori odisli z lode
int *pocet_serf_lod; //pocet serfov ktori odisli z lode
int *pocet_member_lod; //pocetm memberov na lodi
int *mem; //pre udrzanie ID kapitana
int *som_kapitan_hacker; //Flag na zistenie ci je kapitan HACKER/SERF (stacilo urobit klasicku globalnu premennu, ale takto to funguje tiez)
int *som_kapitan_serf; //Flag na zistenie ci je kapitan HACKER/SERF (stacilo urobit klasicku globalnu premennu, ale takto to funguje tiez)
int *cisloriadku; //cislo riadku pre output
int *pocetSerf; //pocet serfov na mole
int *pocetHacker; //pocet hackerov na mole
int *pocetMolo; //celkovy pocet osob na mole
int *closeBoat; //tymto zablokujem semafor a zabranim tym prichod dalsich ludi na lod, resp. vyzbieral som potrebny pocet tak dalsich nepustam
int *pocet_landing; //aby sa dalsi nalodili, az ked su prvi vylodeni

//Miesto pre zdielane premenne
int shm_pocetexit;
int shm_pocetHlod;
int shm_pocetSlod;
int shm_pocetMlod;
int shm_mem;
int shm_kapHack;
int shm_kapSerf;
int shm_akcia; 
int shm_serf;
int shm_hacker;
int shm_pocetmolo;
int shm_closeBoat;
int shm_landing_pocet;

//Funkcia na uzavretie semaforov
void close_semafory()
{
    sem_close(go_on_mole);
    sem_close(sem_output);
    sem_close(mole_wait);
    sem_close(hacker_sorted);
    sem_close(serf_sorted);
    sem_close(captain_wait);
    sem_close(waiting_for_landing);

    //Odalokovanie miesta semaforov
    sem_unlink("/xgeffe00goonmole");
    sem_unlink("/xgeffe00cakanie_ciel");
    sem_unlink("/xgeffe00output");
    sem_unlink("/xgeffe00molowait");
    sem_unlink("/xgeffe00hackersorted");
    sem_unlink("/xgeffe00serfsorted");
    sem_unlink("/xgeffe00captainwait");
    sem_unlink("/xgeffe00volnowait");
}

//Funkcia na odal0ovanie, odmazanie, uzavretie vsetkych zdrojov, ktore sme pouzivali
void dealokacia()
{
    close_semafory();
    fclose(output);

    //Odmapovanie pamate
    munmap(pocet_member_exit, sizeof(int));
    munmap(pocet_hacker_lod, sizeof(int));
    munmap(pocet_serf_lod, sizeof(int));
    munmap(pocet_member_lod, sizeof(int));
    munmap(mem, sizeof(int));
    munmap(som_kapitan_hacker, sizeof(int));
    munmap(som_kapitan_serf, sizeof(int));
    munmap(cisloriadku, sizeof(int));
    munmap(pocetSerf, sizeof(int));
    munmap(pocetHacker, sizeof(int));
    munmap(pocetMolo, sizeof(int));
    munmap(closeBoat, sizeof(int));
    munmap(pocet_landing, sizeof(int));

    //Zmazanie pamate a uzavrenie
    shm_unlink("/xgeffe00pocetexit");
    close(shm_pocetexit);
    shm_unlink("/xgeffe00pocetHlod");
    close(shm_pocetHlod);
    shm_unlink("/xgeffe00pocetSlod");
    close(shm_pocetSlod);
    shm_unlink("/xgeffe00pocetMlod");
    close(shm_pocetMlod);
    shm_unlink("/xgeffe00mem");
    close(shm_mem);
    shm_unlink("/xgeffe00kapHack");
    close(shm_kapHack);
    shm_unlink("/xgeffe00kapSerf");
    close(shm_kapSerf);
    shm_unlink("/xgeffe00akcia");
    close(shm_akcia);
    shm_unlink("/xgeffe00serf");
    close(shm_serf);
    shm_unlink("/xgeffe00hacker");
    close(shm_hacker);
    shm_unlink("/xgeffe00pocetmolo");
    close(shm_pocetmolo);
    shm_unlink("/xgeffe00closeBoat");
    close(shm_closeBoat);
    shm_unlink("/xgeffe00landingpocet");
    close(shm_landing_pocet);
}

// Funkcia na ukoncovanie v pripade chyby
void ukoncenie(int sighandler) {
    (void)sighandler; //kvoli prekladacu
    kill(getpid(), SIGTERM);
    dealokacia();
    exit(CHYBA);
}

//Funkcia na alokovanie zdrojov
void alokacia()
{
    //Vytvorenie semaforov 1nazov semaforu 2flagy 3prava 4pociatocna hodnota
    go_on_mole= sem_open("/xgeffe00goonmole", O_CREAT | O_EXCL, 0644, 0);
    sem_output = sem_open("/xgeffe00output", O_CREAT | O_EXCL, 0644, 1);
    mole_wait = sem_open("/xgeffe00molowait", O_CREAT | O_EXCL, 0644, 0);
    hacker_sorted = sem_open("/xgeffe00hackersorted", O_CREAT | O_EXCL, 0644, 0);
    serf_sorted = sem_open("/xgeffe00serfsorted", O_CREAT | O_EXCL, 0644, 0);
    captain_wait = sem_open("/xgeffe00captainwait", O_CREAT | O_EXCL, 0644, 0);
    waiting_for_landing = sem_open("/xgeffe00volnowait", O_CREAT | O_EXCL, 0644, 1);

    //Ak sa nespravne alokovali, vypis chybu 
    if(sem_output == SEM_FAILED || mole_wait == SEM_FAILED || hacker_sorted == SEM_FAILED || serf_sorted == SEM_FAILED || waiting_for_landing == SEM_FAILED || go_on_mole == SEM_FAILED || captain_wait == SEM_FAILED )
    {
        fprintf(stderr,"Nastala chyba pri alokovani!\n");
        dealokacia();
        exit(CHYBA);
    }

    //Vytvorenie zdielanej pamate 
    shm_pocetexit = shm_open("/xgeffe00pocetexit", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_pocetHlod = shm_open("/xgeffe00pocetHlod", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_pocetSlod = shm_open("/xgeffe00pocetSlod", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_pocetMlod = shm_open("/xgeffe00pocetMlod", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_mem = shm_open("/xgeffe00mem", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_kapHack = shm_open("/xgeffe00kapHack", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_kapSerf = shm_open("/xgeffe00kapSerf", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_akcia = shm_open("/xgeffe00akcia", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_serf = shm_open("/xgeffe00serf", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_hacker = shm_open("/xgeffe00hacker", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_pocetmolo = shm_open("/xgeffe00pocetmolo", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_closeBoat = shm_open("/xgeffe00closeBoat", O_CREAT | O_EXCL | O_RDWR, 0644);
    shm_landing_pocet = shm_open("/xgeffe00landingpocet", O_CREAT | O_EXCL | O_RDWR, 0644);

    //Ak sa nespravne vytvorila zdielana pamat, vytlac na stderr chybu
    if(shm_akcia == ERROR || shm_serf == ERROR || shm_hacker == ERROR || shm_pocetmolo == ERROR || shm_closeBoat == ERROR || shm_pocetexit == ERROR || shm_pocetHlod == ERROR || shm_pocetSlod == ERROR || shm_pocetMlod == ERROR || shm_mem == ERROR || shm_kapHack == ERROR || shm_kapSerf == ERROR || shm_landing_pocet == ERROR)
    {
        fprintf(stderr,"Nastala chyba pri alokovani!\n");
        dealokacia();
        exit(CHYBA);
    }

    //Vytvorenie miesta v zdielanej pamati pre intieger
    ftruncate(shm_pocetexit, sizeof(int));
    ftruncate(shm_pocetHlod, sizeof(int));
    ftruncate(shm_pocetSlod, sizeof(int));
    ftruncate(shm_pocetMlod, sizeof(int));
    ftruncate(shm_mem, sizeof(int));
    ftruncate(shm_kapHack, sizeof(int));
    ftruncate(shm_kapSerf, sizeof(int));
    ftruncate(shm_akcia, sizeof(int));
    ftruncate(shm_serf, sizeof(int));
    ftruncate(shm_hacker, sizeof(int));
    ftruncate(shm_pocetmolo, sizeof(int));
    ftruncate(shm_closeBoat, sizeof(int));
    ftruncate(shm_landing_pocet, sizeof(int));

    //Mapovanie zdielanej pamate
    pocet_member_exit = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_pocetexit, 0);
    pocet_hacker_lod = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_pocetHlod, 0);
    pocet_serf_lod = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_pocetSlod, 0);
    pocet_member_lod = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_pocetMlod, 0);
    mem = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_mem, 0);
    som_kapitan_hacker = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_kapHack, 0);
    som_kapitan_serf = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_kapSerf, 0);
    cisloriadku = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_akcia, 0);
    pocetSerf = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_serf, 0);
    pocetHacker = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_hacker, 0);
    pocetMolo = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_pocetmolo, 0);
    closeBoat = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_closeBoat, 0);
    pocet_landing = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_landing_pocet, 0);

    if(cisloriadku == MAP_FAILED || pocetSerf == MAP_FAILED || pocetHacker == MAP_FAILED || pocetMolo == MAP_FAILED || closeBoat == MAP_FAILED || pocet_landing == MAP_FAILED || pocet_member_exit == MAP_FAILED || pocet_hacker_lod == MAP_FAILED || pocet_serf_lod == MAP_FAILED || pocet_member_lod == MAP_FAILED || mem == MAP_FAILED || som_kapitan_hacker == MAP_FAILED || som_kapitan_serf == MAP_FAILED)
    {
        fprintf(stderr,"Nastala chyba pri vytvarani zdroju!\n");
        dealokacia();
        exit(CHYBA);
    }
}

// Vyberam koho mam pustit do lode
void choosing_members()
{
    if(*pocet_hacker_lod == 4)
    {
        sem_post(hacker_sorted);
        sem_post(hacker_sorted);
        sem_post(hacker_sorted);
        sem_post(hacker_sorted);
        hack4 = 1;
        *pocet_hacker_lod = *pocet_hacker_lod - 4;
        *pocet_member_lod = *pocet_member_lod - 4;
        return ;
    }

    if(*pocet_serf_lod == 4)
    {
        sem_post(serf_sorted);
        sem_post(serf_sorted);
        sem_post(serf_sorted);
        sem_post(serf_sorted);
        serf4 = 1;
        *pocet_serf_lod = *pocet_serf_lod - 4; 
        *pocet_member_lod = *pocet_member_lod - 4;
        return;
    }

    if((*pocet_hacker_lod == 2 && *pocet_serf_lod == 2) || (*pocet_hacker_lod == 2 && *pocet_serf_lod == 3) || (*pocet_hacker_lod == 3 && *pocet_serf_lod == 2))
    {
        sem_post(hacker_sorted);
        sem_post(serf_sorted);
        sem_post(hacker_sorted);
        sem_post(serf_sorted);
        serf_hack = 1;
        *pocet_serf_lod = *pocet_serf_lod - 2;
        *pocet_hacker_lod = *pocet_hacker_lod - 2; 
        *pocet_member_lod = *pocet_member_lod - 4;
        return;
    }
}

//Funkcia na zistenie CAPTAIN alebo MEMBER
int member_or_captain()
{
    if(*pocet_member_lod == 5) //Ak su tam 5 na mole, tak automaticky je 5. clen kapitan, lebo vstupil posledny
    {
        return CAPTAIN;
    }
    else
    {
        if((*pocet_serf_lod == 4) || (*pocet_hacker_lod == 4) || (*pocet_hacker_lod == 2 && *pocet_serf_lod == 2)) //Ak jedna z podmien0 potom kapitan
        {
           return CAPTAIN; 
        }
    }
    return MEMBER;
}

//Funkcia v ktorej vykonava svoje cinnosti Serf
void SerfWork(int poradie, int kapacita, int uspanie, int moloplnespim)
{
    //---------------- Starts ------------------
    sem_wait(sem_output);
    (*cisloriadku)++;
    fprintf(output, "%d: SERF: %d: starts\n", *cisloriadku, poradie);
    sem_post(sem_output);

    //------------- Checking capacity --------------

    /* Tuto sledujem kapacitu na zaklade poctu ludi na mole, ak je plna tak ostatne procesy
    ktore sa tam nezmestia tak sa uspia na nahodnu dobu a po nejakom case sa opatovne skusaju
    dostat na molo 
    * semafor(go_on_mole) - koriguje kolkych procesov moze pustit */

    while(1)
    {
    if (*pocetMolo < kapacita)
    {
        sem_post(go_on_mole);
        (*pocetMolo)++;
        break;
    }
    else
    {	
    	(*cisloriadku)++;
        fprintf(output, "%d: SERF: %d: leaves queue: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
        usleep((rand() % moloplnespim + 1)*1000);
        (*cisloriadku)++;
        fprintf(output, "%d: SERF: %d: is back: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
    }
	}
    sem_wait(go_on_mole); 

    //-------------- Waits ----------------
    /*Ked sa procesy dostanu na molo tak tam cakaju kym sa lod nedoplavi a neurobi miesto
    pre dalsich clenov */

    sem_wait(sem_output);
    (*cisloriadku)++;
    (*pocetSerf)++;
    fprintf(output, "%d: SERF: %d: waits: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
    sem_post(sem_output);

    //-------------- Boarding -------------
    sem_wait(waiting_for_landing);

    if(!(CONDITION_OF_BOAT)) //kontrolujem ci je podienka lode splnena ak nie pustim dalsi proces az kym ju nesplnim
    {
        sem_post(mole_wait);
    }
    sem_wait(mole_wait);

    sem_wait(sem_output);
    (*pocet_serf_lod)++;
    (*pocet_member_lod)++;
    sem_post(sem_output);

    int som = member_or_captain(); //po pusteni 4 alebo 5 procesov (nutne k splneniu podmienky) vyberem na zaklade podmienok ci bude kapitan alebo member

    if(CONDITION_OF_BOAT) //tuto cakaju kym sa lod nedoplavi
    {
         *closeBoat = CLOSE;
    }

    if(*closeBoat == OPEN) //neplavia sa tak posli proces dalej, a pust dalsich ludi
    {
        sem_post(waiting_for_landing);
    } 

    if(CONDITION_OF_BOAT) //tuto selektujem pocet procesov na 4, pretoze na lod mozem pustit vzdy len 4, no mozu tam byt 5 ak sa pri 4 procesoch podmienka nesplnila
    {
        choosing_members();
    }
    sem_wait(serf_sorted);

    //----------- CAPTAIN BOARDS/SLEEPING ---------------
    sem_wait(sem_output);

    if(som == CAPTAIN) //Kapitan vypise boards
    {
    	//Znizovanie poctu hackerov, serfov, resp. uvolnenie kapacity, aby som mohol pustit dalsich na molo
        (*cisloriadku)++;
        if(hack4 == 1)
        {
        	*pocetHacker = *pocetHacker - 4;
        	hack4 = 0;
        }
        else if (serf4 == 1)
        {
        	*pocetSerf = *pocetSerf - 4;
        	serf4 = 0;
        }
        else if (serf_hack == 1)
        {
        	*pocetHacker = *pocetHacker - 2;
        	*pocetSerf = *pocetSerf - 2;
        	serf_hack = 0;
        }             
        fprintf(output, "%d: SERF: %d: boards: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
        *pocetMolo = *pocetMolo - 4;     
        (*som_kapitan_serf) = 1;
        (*mem) = poradie;
    }
    sem_post(sem_output);

    if(som == CAPTAIN) //Ostatni cakaju kym dojde kapitan a naraz vsetci spolu pojdu do spanku
    {
        usleep((rand() % uspanie + 1)*1000);
        sem_post(captain_wait);
        sem_post(captain_wait);
        sem_post(captain_wait);
        sem_post(captain_wait);
    }
    sem_wait(captain_wait);

    //------------------ Finish --------------------
    sem_wait(sem_output);
    (*pocet_landing)++;
    (*cisloriadku)++;

    if (som != CAPTAIN) //Memberi vypisu exits, zvysuju pocet leavnutych memberov o 1, ak sa pocet leavnutych memberov rovna 3 tak potom odide kapitan, pretoze musi odchadzat posledny
    {
        fprintf(output, "%d: SERF: %d: member exits: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
        (*pocet_member_exit)++;
        if (*pocet_member_exit == 3)
        {
            if (*som_kapitan_serf == 1) 
            {
                fprintf(output, "%d: SERF: %d: captain exits: %d: %d\n", *cisloriadku, *mem, *pocetHacker, *pocetSerf);
            }
            else
            {
                fprintf(output, "%d: HACK: %d: captain exits: %d: %d\n", *cisloriadku, *mem, *pocetHacker, *pocetSerf);
            }
            (*pocet_member_exit) = 0;
            (*som_kapitan_serf) = 0;
            (*som_kapitan_hacker) = 0;
        }

    }

    if (*pocet_landing == 4) //pockaju sa vsetky 4 procesy na konci a pustia dalsie procesy na lod
    {
        *pocet_landing = 0;
        *closeBoat = OPEN;      
        sem_post(waiting_for_landing);
    }
    sem_post(sem_output);
    close_semafory();
}

//Funkcia v ktorej vykonava svoje cinnosti Hacker
void HackerWork(int poradie, int kapacita, int uspanie, int moloplnespim)
{
//Funkcia velmi podobna SerfWork

	//---------------Started-----------------
    sem_wait(sem_output);
    (*cisloriadku)++;
    fprintf(output, "%d: HACK: %d: starts\n", *cisloriadku, poradie);
    sem_post(sem_output);

    //--------------Checking capacity--------------
    while(1)
    {
    if (*pocetMolo < kapacita){
        sem_post(go_on_mole);
        (*pocetMolo)++;
        break;
    }
    else
    {
        (*cisloriadku)++;

        fprintf(output, "%d: HACK: %d: leaves queue: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
        usleep((rand() % moloplnespim + 1)*1000);
        (*cisloriadku)++;
        fprintf(output, "%d: HACK: %d: is back: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
    }
	}
    sem_wait(go_on_mole);

    //-------------- Waits ----------------
    sem_wait(sem_output);
    (*cisloriadku)++;
    (*pocetHacker)++;
    fprintf(output, "%d: HACK: %d: waits: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
    sem_post(sem_output);

    //-------------- Boarding -------------
    sem_wait(waiting_for_landing);

    if(!(CONDITION_OF_BOAT))
    {
        sem_post(mole_wait);
    }
    sem_wait(mole_wait);

    sem_wait(sem_output);
    (*pocet_hacker_lod)++;
    (*pocet_member_lod)++;
    sem_post(sem_output);

    int som = member_or_captain();

    if(CONDITION_OF_BOAT) 
    {
         *closeBoat = CLOSE;
    }

    if(*closeBoat == OPEN)
    {
        sem_post(waiting_for_landing);
    } 

    if(CONDITION_OF_BOAT)
    {
        choosing_members();
    }
    sem_wait(hacker_sorted);

    //----------- CAPTAIN BOARDS/SLEEPING ---------------
    sem_wait(sem_output);

    if(som == CAPTAIN) 
    {
        (*cisloriadku)++;
        if(hack4 == 1)
        {
        	*pocetHacker = *pocetHacker - 4;
        	hack4 = 0;
        }
        else if (serf4 == 1)
        {
        	*pocetSerf = *pocetSerf - 4;
        	serf4 = 0;
        }
        else if (serf_hack == 1)
        {
        	*pocetHacker = *pocetHacker - 2;
        	*pocetSerf = *pocetSerf - 2;
        	serf_hack = 0;
        }        
        fprintf(output, "%d: HACK: %d: boards: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
        *pocetMolo = *pocetMolo - 4;
        (*som_kapitan_hacker)=1;
        (*mem) = poradie;
    }
    sem_post(sem_output);

    if(som == CAPTAIN) 
    {
        usleep((rand() % uspanie + 1)*1000);
        sem_post(captain_wait);
        sem_post(captain_wait);
        sem_post(captain_wait);
        sem_post(captain_wait);
    }
    sem_wait(captain_wait);

    //------------------Finish----------------------
    sem_wait(sem_output);
    (*pocet_landing)++;
    (*cisloriadku)++;

    if (som != CAPTAIN)
    {
        fprintf(output, "%d: HACK: %d: member exits: %d: %d\n", *cisloriadku, poradie, *pocetHacker, *pocetSerf);
        (*pocet_member_exit)++;

        if (*pocet_member_exit == 3)
        {
            if (*som_kapitan_hacker == 1) 
            {
                fprintf(output, "%d: HACK: %d: captain exits: %d: %d\n", *cisloriadku, *mem, *pocetHacker, *pocetSerf);
            }
            else
            {
                fprintf(output, "%d: SERF: %d: captain exits: %d: %d\n", *cisloriadku, *mem, *pocetHacker, *pocetSerf);
            }
            (*pocet_member_exit) = 0;
            (*som_kapitan_serf) = 0;
            (*som_kapitan_hacker) = 0;
        }
    }

    if (*pocet_landing == 4) 
    {
        *pocet_landing = 0;
        *closeBoat = OPEN;
        sem_post(waiting_for_landing);
    }
    sem_post(sem_output);
    close_semafory();
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, ukoncenie); //odchytenie signalov a nasledne volanie funkcie
    signal(SIGINT, ukoncenie); 
    srand(time(NULL)); // nahodne cislo

    //Argumenty
    int P;
    int H;
    int S;
    int R;
    int W;
    int C;
    char *ptr;

    if (argc == 7) 
    {
    	//------ Checking if arguments are intiegers ------

    	P = strtol(argv[1], &ptr, 10);
    	if(*ptr != '\0')
    	{
    		fprintf(stderr, "Argument musi byt intieger!");
        	return CHYBA;
    	}
        H = strtol(argv[2], &ptr, 10);
        if(*ptr != '\0')
    	{
    		fprintf(stderr, "Argument musi byt intieger!");
        	return CHYBA;
    	}
        S = strtol(argv[3], &ptr, 10);
        if(*ptr != '\0')
    	{
    		fprintf(stderr, "Argument musi byt intieger!");
        	return CHYBA;
    	}
   	    R = strtol(argv[4], &ptr, 10);
   	    if(*ptr != '\0')
    	{
    		fprintf(stderr, "Argument musi byt intieger!");
        	return CHYBA;
    	}
        W = strtol(argv[5], &ptr, 10);
        if(*ptr != '\0')
    	{
    		fprintf(stderr, "Argument musi byt intieger!");
        	return CHYBA;
    	}
   	    C = strtol(argv[6], &ptr, 10);
   	    if(*ptr != '\0')
    	{
    		fprintf(stderr, "Argument musi byt intieger!");
        	return CHYBA;
    	}

    	//------------------ CONDITIONS OF ARGUMENTS -----------------------

        if (( (P%2) != 0 ) || P <= 0 )
        {
            fprintf(stderr, "Pocet osob musi byt delitelny dvomi!");
            return CHYBA;
        }

        if( (H < 0) || (H > 2001)) 
        {
            fprintf(stderr, "Cas pre Hackera nebol zadany spravne!");
            return CHYBA;
        }
        if( (S < 0) || (S > 2001)) 
        {
            fprintf(stderr, "Cas pre Serfa nebol zadany spravne!");
            return CHYBA;
        }
        if((R < 0) || (R > 2001))
        {
            fprintf(stderr, "Cas plavby nebol zadany spravne!");
            return CHYBA;
        }
        if((W < 20) || (W > 2001))
        {
            fprintf(stderr, "Cas prichodu HACKERA/SERFA nebol zadany spravne!");
            return CHYBA;
        }
        if((C < 5) || (C > 2001)) 
        {
            fprintf(stderr, "Kapacita nebola zadana spravne!");
            return CHYBA;
        }
    }
    else //AK neni 7 argumentov, vyhod chybu
    {
            fprintf(stderr, "Argumenty neboli zadane spravne!");
            return CHYBA;
    }

    if ((output = fopen("proj2.out", "w"))== NULL) // OPEN file for output
    {
        fprintf(stderr,"Subor sa nepodarilo otvorit!\n");
        return CHYBA;
    }
    setbuf(output, NULL); //nastavenie bufferu aby sa outputovalo do suboru po riadkoch

    alokacia();

    //-------------------- Serfs ---------------------
    //Vytvaranie Serfov
    pid_t Serf;
    pid_t MainSerf = fork(); //vytvorime child proces MainSerf

    if (MainSerf == CHILD) 
    {
        for (int j = 0; j < P; ++j) //v cykle vytvaram procesy serfov 
        {
            if (S > 0) 
            {
                usleep((rand() % S + 1)*1000); //uspanie na nahodnu dobu
            }
            if (S == 0) //Ked sa cas na spawnutie serfa == 0
            {
                usleep(0);
            }
            Serf = fork(); //vytvorime child process Serf
            if (Serf == CHILD)
            {
                SerfWork(j + 1, C, R, W);
                fclose(output);
                exit(0);
            }
            if(Serf < CHILD) //Child nebol spravne vytvoreny tak chyba a zabijem uz vytvorene procesy
            {
                fprintf(stderr,"Nepodarilo sa vytvorit potomka Serf!\n");
                kill(MainSerf, SIGTERM);
                ukoncenie(0);
                exit(CHYBA);
            }
        }
        if(Serf > CHILD) 
        {
            while(wait(NULL) > 0); //cakanie
        }
        close_semafory();
        fclose(output);
        exit(0);
    }
    if(MainSerf < CHILD)
    {
        fprintf(stderr,"Nepodaril sa vytvorit potomka!\n");
        kill(MainSerf, SIGTERM);
        ukoncenie(0);
        exit(CHYBA);
    }

    //---------------- Hackers -------------------
    //Vytvaranie Hackerov  
    pid_t Hacker; 
    pid_t MainHacker = fork(); //vytvorime child proces MainHacker
    if (MainHacker == CHILD) 
    {
        for (int i = 0; i < P; ++i) // v cykle vytvaram procesy hackerov 
        {
            if(H > 0)
            {
                usleep((rand() % H + 1)*1000); //uspanie na nahodnu dobu
            }
            if(H == 0) //Ked sa cas na spawnutie hackera == 0
            {
                usleep(0);
            }

            Hacker = fork(); //vytvorime child process Hacker
            if (Hacker == CHILD) 
            {
                HackerWork(i + 1, C, R, W);
                fclose(output);
                exit(0);
            }
            if(Hacker < CHILD) //Child nebol spravne vytvoreny tak chyba a zabijem uz vytvorene procesy
            {
                fprintf(stderr,"Nepodaril sa vytvorit potomok Hacker\n");
                kill(MainHacker, SIGTERM);
                ukoncenie(0);
                exit(CHYBA);
            }
        }
        if(Hacker > CHILD) 
        {
            while(wait(NULL) > 0); //cakanie
        }
        close_semafory();
        fclose(output);
        exit(0);
    }
    if(MainHacker < CHILD)
    {
        fprintf(stderr,"Nepodaril sa vytvorit potomok!\n");
        kill(MainHacker, SIGTERM);
        ukoncenie(0);
        exit(CHYBA);
    }

    if((MainHacker > CHILD) && (MainSerf > CHILD)) //cakanie na deti hlavneho procesu
    {
        while(wait(NULL) > 0); //cakam
    }

    dealokacia();
    return 0;
}

