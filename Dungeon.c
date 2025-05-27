#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Constantes voor de stats van monsters
#define BABY_DRAGON_HEALTH 200        // HP van baby draak
#define ANCIENT_DRAGON_HEALTH 400     // HP van oude draak
#define BABY_DRAGON_DAMAGE 2          // Schade van baby draak
#define ANCIENT_DRAGON_DAMAGE 5       // Schade van oude draak

// Struct voor een verbinding tussen kamers (linked list)
typedef struct RoomLink {
    struct Room* connected_room;    // Pointer naar de verbonden kamer
    struct RoomLink* next;          // Volgende verbinding in de lijst
} RoomLink;

// Struct voor een kamer in de dungeon
typedef struct Room {
    int id;                        // Unieke kamer-ID
    int door_count;                // Aantal deuren (verbindingen)
    int has_monster;               // 0 = geen monster, 1 = baby draak, 2 = oude draak
    int searched;                  // 0 = niet doorzocht, 1 = wel doorzocht
    _Bool visited;                 // Bezocht of niet
    _Bool cleared;                 // Kamer is gezuiverd (monster verslagen)
    _Bool has_treasure;            // Heeft kamer schat (Gouden Ei)?
    RoomLink* links;               // Lijst van verbonden kamers (deuren)
} Room;

// Struct voor de hele dungeon, met aantal kamers en array van kamers
typedef struct {
    int room_count;                // Hoeveelheid kamers
    Room* rooms;                  // Array met kamers
} Dungeon;

// Globale variabelen voor gevechtslogica
int monster_attack = 0;           // Flag: monster valt aan
int player_attack = 0;            // Flag: speler valt aan
int random_num = 0;               // Willekeurig getal voor gevecht
int game_ended = 0;               // Flag: is het spel voorbij?

// Struct voor speler
typedef struct {
    char name[50];                // Naam speler
    int health;                   // HP speler
    int attack_power;             // Aanvalskracht speler
    int current_room;             // Huidige kamer waarin speler zich bevindt
} Player;

// Struct voor monster
typedef struct {
    char* name;                   // Naam monster
    int health;                   // HP monster
    int attack_power;             // Aanvalskracht monster
} Monster;

Player player;                    // Globale speler variabele
Monster baby_dragon;             // Monster baby draak
Monster ancient_dragon;          // Monster oude draak

// Functie pointers voor acties: monster aanmaken en gevechten starten
typedef void (*MonsterGenerator)(Room*);
typedef void (*BattleHandler)(Monster*, Room*);
typedef void (*ItemHandler)(Room*);

void create_monsters(Room *room);        // Monster generatie functie
void start_battle(Monster *monster, Room *room);  // Gevechtsfunctie

MonsterGenerator generate_monster = &create_monsters;  // Pointer naar monster aanmaak
BattleHandler fight = &start_battle;                     // Pointer naar gevecht functie

// Item acties: functie voor gezondheidsdrankje
void health_potion(Room *room) {
    int heal = 20 + rand() % 31;   // Heal tussen 20 en 50 HP
    player.health += heal;          // Voeg toe aan speler HP
    printf("\nGevonden: gezondheidsdrankje met drakenspeeksel! +%d HP!\n", heal);
    printf("Huidige HP: %d\n", player.health);
}

// Item actie: power-up (verdubbel aanval)
void power_up(Room *room) {
    int original_power = player.attack_power;
    player.attack_power *= 2;
    printf("\nGevonden: Magisch VuurZwaard! Aanvalskracht verhoogd van %d naar %d!\n", original_power, player.attack_power);
}

// Item actie: valstrik, speler verliest HP
void trap(Room *room) {
    int damage = 20 + rand() % 31; // Schade tussen 20 en 50 HP
    player.health -= damage;
    printf("\nAUW! Je stapte op een Drakenschub! -%d HP!\n", damage);
    printf("Huidige HP: %d\n", player.health);
}

// Array met pointers naar item functies (voor willekeurige selectie)
ItemHandler item_actions[3] = {
    &health_potion,
    &power_up,
    &trap
};

// Monster generatie in kamer, op basis van kans
void create_monsters(Room *room){
    if ((rand() % 4) == 0){               // 25% kans op monster
        if ((rand() % 3) == 0){           // 33% kans dat monster een oude draak is
            room->has_monster = 2;
        }
        else{
            room->has_monster = 1;        // Anders baby draak
        }
    }
    else{
        room->has_monster = 0;            // Geen monster
    }
}

// Verbinding maken tussen twee kamers (deuren)
void link_rooms(Room* room1, Room* room2){
    // Voorkom dubbele verbinding tussen dezelfde kamers
    RoomLink* current = room1->links;
    while (current != NULL) {
        if (current->connected_room == room2) return;  // Verbinding bestaat al
        current = current->next;
    }

    // Maak nieuwe link van kamer1 naar kamer2
    RoomLink* link1 = malloc(sizeof(RoomLink));
    link1->connected_room = room2;
    link1->next = room1->links;
    room1->links = link1;
    room1->door_count++;

    // Maak nieuwe link van kamer2 naar kamer1 (tweezijdige verbinding)
    RoomLink* link2 = malloc(sizeof(RoomLink));
    link2->connected_room = room1;
    link2->next = room2->links;
    room2->links = link2;
    room2->door_count++;
}

// Vind beschikbare kamer met minder dan 4 deuren (voor verbinding)
int find_available_room(Dungeon* dungeon, int current_id){
    int available_rooms[dungeon->room_count];   // Array om beschikbare kamers te bewaren
    int count = 0;

    // Kijk in alle kamers vóór huidige (0 tot current_id-1)
    for(int i = 0; i < current_id; i++){
        if(dungeon->rooms[i].door_count < 4){   // Minder dan 4 deuren = beschikbaar
            available_rooms[count++] = i;
        }
    }
    if(count > 0){
        return available_rooms[rand() % count];  // Kies willekeurige beschikbare kamer
    }
    else{
        return -1;  // Geen kamer beschikbaar
    }
}

// Bouw de dungeon met het gevraagde aantal kamers
Dungeon* build_dungeon(int room_count){
    Dungeon* dungeon = malloc(sizeof(*dungeon));   // Allocate geheugen voor dungeon
    dungeon->room_count = room_count;
    dungeon->rooms = malloc(room_count * sizeof(Room)); // Allocate array kamers

    // Initialiseer elke kamer
    for (int i = 0; i < room_count; i++) {
        dungeon->rooms[i].id = i;
        dungeon->rooms[i].door_count = 0;
        dungeon->rooms[i].links = NULL;
        dungeon->rooms[i].visited = 0;
        dungeon->rooms[i].cleared = 0;
        dungeon->rooms[i].searched = 0;
        dungeon->rooms[i].has_treasure = 0;

        (*generate_monster)(&dungeon->rooms[i]);  // Genereer monster in kamer
    }

    // Kies een kamer met schat (Gouden Ei)
    int treasure_room = rand() % room_count;
    dungeon->rooms[treasure_room].has_treasure = 1;

    // Maak verbindingen tussen kamers
    for(int i = 1; i < room_count; i++){
        int target = find_available_room(dungeon, i);  // Zoek kamer om mee te verbinden
        if (target == -1){                              // Geen kamer gevonden, faal
            free(dungeon->rooms);
            free(dungeon);
            return NULL;
        }
        link_rooms(&dungeon->rooms[i], &dungeon->rooms[target]);  // Verbinden

        int extra_links = rand() % 4;          // Voeg extra verbindingen toe (0-3)
        for (int j = 0; j < extra_links; j++){
            int new_target = find_available_room(dungeon, i);
            if (new_target != -1 && dungeon->rooms[i].door_count < 4){
                link_rooms(&dungeon->rooms[i], &dungeon->rooms[new_target]);
            }
        }
    }
    return dungeon;
}

// Ruim geheugen op bij einde dungeon
void destroy_dungeon(Dungeon* dungeon){
    for (int i = 0; i < dungeon->room_count; i++){
        RoomLink* current = dungeon->rooms[i].links;
        while (current != NULL) {
            RoomLink* temp = current;
            current = current->next;
            free(temp);             // Free linked list van verbindingen
        }
    }
    free(dungeon->rooms);          // Free array kamers
    free(dungeon);                 // Free dungeon struct
}

// Binair gevechtssysteem: bepaalt wie aanvalt op basis van binaire representatie van random_num
void binary_fight(){
    int binary[10];        // Array voor bits (max 10 bits)
    int i = 0;
    int temp = random_num; // Gebruik temp variabele, want random_num wordt aangepast in loop
    while (temp > 0){
        binary[i] = temp % 2;  // Bepaal bit 0 of 1
        temp = temp / 2;
        i++;
    }
    for(int j = i - 1; j >= 0; j--){
        if (binary[j] == 0){
            monster_attack = 1;  // Monster valt aan als bit 0
            player_attack = 0;
        }
        else if(binary[j] == 1){
            monster_attack = 0;  // Speler valt aan als bit 1
            player_attack = 1;
        }
    }
}

// Verwerk items in kamer bij eerste keer doorzoeken
void process_items(Room *current_room) {
    if(current_room->searched == 0){
        current_room->searched = 1;          // Markeer kamer als doorzocht
        
        if(current_room->has_treasure) {     // Als kamer schat heeft
            printf("\nGevonden: het Gouden DrakenEI!\n");
            printf("Gewonnen!! Jij bent nu de rijkste man ter wereld!\n");
            game_ended = 1;                  // Einde spel
            remove("save.dat");              // Verwijder opgeslagen spel
            return;
        }
        if(rand() % 4 == 0) {                // 25% kans op item
            int item_type = rand() % 3;      // Kies willekeurig item type (0-2)
            (*item_actions[item_type])(current_room);  // Voer item actie uit
        }
    }
}

// Speler valt monster aan
void player_attack_action(Monster *m, Room *r) {
    m->health -= player.attack_power;       // Verminder monster HP met aanvalskracht speler
    printf("SWOOSH! Je raakte de %s! HP: %d.\n", m->name, m->health);
}

// Monster valt speler aan
void monster_attack_action(Monster *m, Room *r) {
    player.health -= (m->attack_power);     // Verminder speler HP met aanvalskracht monster
    printf("De %s vuurt vuur op je af! Pas op %s! HP: %d.\n", m->name, player.name, player.health);
}

// Start gevecht tussen speler en monster in huidige kamer
void start_battle(Monster *m, Room *current_room){
    printf("Pas op!! Een %s!\n", m->name);
    while(m->health > 0 && player.health > 0){
        random_num = rand() % 17;    // Willekeurig getal 0-16
        binary_fight();              // Bepaal wie aanvalt
        if(monster_attack == 1){
            monster_attack_action(m, current_room);
        }
        if(player_attack == 1){
            player_attack_action(m, current_room);
        }
    }

    if(m->health <= 0){              // Monster verslagen
        printf("De %s is verslagen!\n", m->name);
        current_room->cleared = 1;   // Kamer is gezuiverd
    }
    if(player.health <= 0){          // Speler verslagen
        printf("%s is verslagen...\n", player.name);
        game_ended = 1;              // Einde spel
    }
    if(current_room->cleared == 1){  // Beloning na gevecht
        printf("Je vond genezende kruiden en kreeg 100 HP terug!\n");
        player.health += 100;
        printf("Huidige HP: %d.\n", player.health);
    }
}

// Placeholder voor save game functie (implementatie niet gegeven)
void save_game(Dungeon *d, Player *p) { /* zelfde als voorheen */ }

// Placeholder voor load game functie (implementatie niet gegeven)
int load_game(Dungeon **d, Player *p) { /* zelfde als voorheen */ return 0; }

int main(){
    srand(time(NULL));           // Initialiseer random generator met tijd

    Dungeon* dungeon = NULL;

    // Controleer of er een opgeslagen spel is
    FILE *save_file = fopen("save.dat", "rb");
    if (save_file) {
        fclose(save_file);
        printf("Opgeslagen spel gevonden. Laden? (1=Ja, 0=Nee): ");
        int choice;
        scanf("%d", &choice);
        while(getchar() != '\n');  // Buffer leegmaken

        if (choice == 1) {
            if (!load_game(&dungeon, &player)) {  // Probeer opgeslagen spel te laden
                printf("Fout bij laden. Nieuw spel starten.\n");
            }
        }
    }

    if (!dungeon) {  // Geen spel geladen => nieuw spel starten       
        printf("Voer je naam in: ");
        fgets(player.name, sizeof(player.name), stdin);  // Lees naam speler in
        player.name[strcspn(player.name, "\n")] = '\0'; // Verwijder newline uit naam

        printf("Welkom %s, veel plezier in de DrakenGrot opzoek naar het Gouden EI!\n", player.name);

        player.health = 100;           // Start HP speler
        player.attack_power = 5;       // Start aanvalskracht speler

        int room_count;
        printf("Hoeveel kamers wil je? (2-25): ");
        scanf("%d", &room_count);
        while(getchar() != '\n');      // Buffer leegmaken
        
        if (room_count < 2 || room_count > 25) {
            room_count = 10;           // Default waarde als ongeldige input
            printf("Onjuiste waarde, aantal kamers is nu 10.\n");
        }

        dungeon = build_dungeon(room_count);
        if (!dungeon) {
            printf("Fout bij het bouwen van de dungeon.\n");
            return 1;
        }
        player.current_room = 0;       // Start in kamer 0
    }

    baby_dragon.name = "Baby Draak";         // Definieer baby draak
    baby_dragon.health = BABY_DRAGON_HEALTH;
    baby_dragon.attack_power = BABY_DRAGON_DAMAGE;

    ancient_dragon.name = "Oude Draak";      // Definieer oude draak
    ancient_dragon.health = ANCIENT_DRAGON_HEALTH;
    ancient_dragon.attack_power = ANCIENT_DRAGON_DAMAGE;

    // Hoofdlus van het spel
    while(!game_ended){
        Room *current_room = &dungeon->rooms[player.current_room];

        printf("\nJe bent nu in kamer %d\n", current_room->id);
        printf("Je gezondheid is %d\n", player.health);

        // Als monster aanwezig en kamer niet gezuiverd, start gevecht
        if(current_room->has_monster && !current_room->cleared){
            Monster *monster = (current_room->has_monster == 1) ? &baby_dragon : &ancient_dragon;
            monster->health = (current_room->has_monster == 1) ? BABY_DRAGON_HEALTH : ANCIENT_DRAGON_HEALTH;
            (*fight)(monster, current_room);    // Start gevecht
            if(game_ended) break;
        }
        else {
            printf("Deze kamer is rustig...\n");
        }

        process_items(current_room);          // Verwerk items/schatten in kamer
        if(game_ended) break;

        // Toon beschikbare verbindingen
        printf("Verbonden kamers:\n");
        RoomLink* link = current_room->links;
        int count = 0;
        while (link) {
            printf("[%d] Kamer %d\n", count, link->connected_room->id);
            link = link->next;
            count++;
        }

        if(count == 0) {
            printf("Geen verbindingen, einde spel.\n");
            break;
        }

        // Vraag speler waar naartoe te gaan
        int choice;
        printf("Naar welke kamer wil je gaan? (nummer ingeven): ");
        scanf("%d", &choice);
        while(getchar() != '\n');

        if(choice < 0 || choice >= count){
            printf("Ongeldige keuze, probeer opnieuw.\n");
            continue;
        }

        // Verplaats speler naar gekozen kamer
        link = current_room->links;
        for (int i = 0; i < choice; i++) {
            link = link->next;
        }
        player.current_room = link->connected_room->id;

        // Optioneel: sla spel op
        printf("Wil je het spel opslaan? (1=Ja, 0=Nee): ");
        int save_choice;
        scanf("%d", &save_choice);
        while(getchar() != '\n');

        if(save_choice == 1){
            save_game(dungeon, &player);
            printf("Spel opgeslagen!\n");
        }
    }

    printf("\nEinde spel.\n");
    destroy_dungeon(dungeon);   // Ruim geheugen op
    return 0;
}
