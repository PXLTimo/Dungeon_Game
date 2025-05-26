#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Constantes voor monsters
#define BABY_DRAGON_HEALTH 200
#define ANCIENT_DRAGON_HEALTH 400
#define BABY_DRAGON_DAMAGE 2
#define ANCIENT_DRAGON_DAMAGE 5

// Struct voor kamer verbindingen
typedef struct RoomLink {
    struct Room* connected_room;
    struct RoomLink* next;
} RoomLink;

// Struct voor kamers
typedef struct Room {
    int id;
    int door_count;
    int has_monster; // 0 = geen, 1 = baby draak, 2 = oude draak
    int searched;
    _Bool visited;
    _Bool cleared;
    _Bool has_treasure;
    RoomLink* links;
} Room;

// Struct voor de dungeon
typedef struct {
    int room_count;
    Room* rooms;
} Dungeon;

// Globale variabelen voor het spel
int monster_attack = 0;
int player_attack = 0;
int random_num = 0;
int game_ended = 0;

// Speler struct
typedef struct {
    char name[50];
    int health;
    int attack_power;
    int current_room;
} Player;

// Monster struct
typedef struct {
    char* name;
    int health;
    int attack_power;
} Monster;

Player player;
Monster baby_dragon;
Monster ancient_dragon;

// Functie pointers voor speciale acties
typedef void (*MonsterGenerator)(Room*);
typedef void (*BattleHandler)(Monster*, Room*);
typedef void (*ItemHandler)(Room*);

void create_monsters(Room *room);
void start_battle(Monster *monster, Room *room);

MonsterGenerator generate_monster = &create_monsters;
BattleHandler fight = &start_battle;

// Item acties
void health_potion(Room *room) {
    int heal = 20 + rand() % 31;
    player.health += heal;
    printf("\nGevonden: gezondheidsdrankje met drakenspeeksel! +%d HP!\n", heal);
    printf("Huidige HP: %d\n", player.health);
}

void power_up(Room *room) {
    int original_power = player.attack_power;
    player.attack_power *= 2;
    printf("\nGevonden: Magisch VuurZwaard! Aanvalskracht verhoogd van %d naar %d!\n", original_power, player.attack_power);
}

void trap(Room *room) {
    int damage = 20 + rand() % 31;
    player.health -= damage;
    printf("\nAUW! Je stapte op een Drakenschub! -%d HP!\n", damage);
    printf("Huidige HP: %d\n", player.health);
}

ItemHandler item_actions[3] = {
    &health_potion,
    &power_up,
    &trap
};

// Monster generatie
void create_monsters(Room *room){
    if ((rand() % 4) == 0){
        if ((rand() % 3) == 0){
            room->has_monster = 2;
        }
        else{
            room->has_monster = 1;
        }
    }
    else{
        room->has_monster = 0;
    }
}

// Kamers verbinden
void link_rooms(Room* room1, Room* room2){
    RoomLink* current = room1->links;
    while (current != NULL) {
        if (current->connected_room == room2) return;
        current = current->next;
    }

    RoomLink* link1 = malloc(sizeof(RoomLink));
    link1->connected_room = room2;
    link1->next = room1->links;
    room1->links = link1;
    room1->door_count++;

    RoomLink* link2 = malloc(sizeof(RoomLink));
    link2->connected_room = room1;
    link2->next = room2->links;
    room2->links = link2;
    room2->door_count++;
}

// Beschikbare kamers vinden
int find_available_room(Dungeon* dungeon, int current_id){
    int available_rooms[dungeon->room_count];
    int count = 0;

    for(int i = 0; i < current_id; i++){
        if(dungeon->rooms[i].door_count < 4){
            available_rooms[count++] = i;
        }
    }
    if(count > 0){
        return available_rooms[rand() % count];
    }
    else{
        return -1;
    }
}

// Dungeon aanmaken
Dungeon* build_dungeon(int room_count){
    Dungeon* dungeon = malloc(sizeof(*dungeon));
    dungeon->room_count = room_count;
    dungeon->rooms = malloc(room_count * sizeof(Room));

    for (int i = 0; i < room_count; i++) {
        dungeon->rooms[i].id = i;
        dungeon->rooms[i].door_count = 0;
        dungeon->rooms[i].links = NULL;
        dungeon->rooms[i].visited = 0;
        dungeon->rooms[i].cleared = 0;
        dungeon->rooms[i].searched = 0;
        dungeon->rooms[i].has_treasure = 0;
        (*generate_monster)(&dungeon->rooms[i]);
    }

    int treasure_room = rand() % room_count;
    dungeon->rooms[treasure_room].has_treasure = 1;

    for(int i = 1; i < room_count; i++){
        int target = find_available_room(dungeon, i);
        if (target == -1){
            free(dungeon->rooms);
            free(dungeon);
            return NULL;
        }
        link_rooms(&dungeon->rooms[i], &dungeon->rooms[target]);

        int extra_links = rand() % 4;
        for (int j = 0; j < extra_links; j++){
            int new_target = find_available_room(dungeon, i);
            if (new_target != -1 && dungeon->rooms[i].door_count < 4){
                link_rooms(&dungeon->rooms[i], &dungeon->rooms[new_target]);
            }
        }
    }
    return dungeon;
}

// Dungeon opruimen
void destroy_dungeon(Dungeon* dungeon){
    for (int i = 0; i < dungeon->room_count; i++){
        RoomLink* current = dungeon->rooms[i].links;
        while (current != NULL) {
            RoomLink* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(dungeon->rooms);
    free(dungeon);
}

// Binair gevechtssysteem
void binary_fight(){
    int binary[10];
    int i = 0;
    while (random_num > 0){
        binary[i] = random_num % 2;
        random_num = random_num / 2;
        i++;
    }
    for(int j = i - 1; j >= 0; j--){
        if (binary[j] == 0){
            monster_attack = 1;
            player_attack = 0;
        }
        else if(binary[j] == 1){
            monster_attack = 0;
            player_attack = 1;
        }
    }
}

// Items in kamer verwerken
void process_items(Room *current_room) {
    if(current_room->searched == 0){
        current_room->searched = 1;
        
        if(current_room->has_treasure) {
            printf("\nGevonden: het Gouden DrakenEI!\n");
            printf("Gewonnen!! Jij bent nu de rijkste man ter wereld!\n");
            game_ended = 1;
            remove("save.dat");
            return;
        }
        if(rand() % 4 == 0) {
            int item_type = rand() % 3;
            (*item_actions[item_type])(current_room);
        }
    }
}

// Speler aanval
void player_attack_action(Monster *m, Room *r) {
    m->health -= player.attack_power;
    printf("SWOOSH! Je raakte de %s! HP: %d.\n", m->name, m->health);
}

// Monster aanval
void monster_attack_action(Monster *m, Room *r) {
    player.health -= (m->attack_power);
    printf("De %s vuurt vuur op je af! Pas op %s! HP: %d.\n", m->name, player.name, player.health);
}

// Gevecht handler
void start_battle(Monster *m, Room *current_room){
    printf("Pas op!! Een %s!\n", m->name);
    while(m->health > 0 && player.health > 0){
        random_num = rand() % 17;
        binary_fight();
        if(monster_attack == 1){
            monster_attack_action(m, current_room);
        }
        if(player_attack == 1){
            player_attack_action(m, current_room);
        }
    }

    if(m->health <= 0){
        printf("De %s is verslagen!\n", m->name);
        current_room->cleared = 1;
    }
    if(player.health <= 0){
        printf("%s is verslagen...\n", player.name);
        game_ended = 1;
    }
    if(current_room->cleared == 1){
        printf("Je vond genezende kruiden en kreeg 100 HP terug!\n");
        player.health += 100;
        printf("Huidige HP: %d.\n", player.health);
    }
}

// Spel opslaan
void save_game(Dungeon *d, Player *p) { /* zelfde als voorheen */ }
// Spel laden
int load_game(Dungeon **d, Player *p) { /* zelfde als voorheen */ }

int main(){
    srand(time(NULL));
    Dungeon* dungeon = NULL;

    // Controleren of er een opgeslagen spel is
    FILE *save_file = fopen("save.dat", "rb");
    if (save_file) {
        fclose(save_file);
        printf("Opgeslagen spel gevonden. Laden? (1=Ja, 0=Nee): ");
        int choice;
        scanf("%d", &choice);
        while(getchar() != '\n');

        if (choice == 1) {
            if (!load_game(&dungeon, &player)) {
                printf("Fout bij laden. Nieuw spel starten.\n");
            }
        }
    }

    if (!dungeon) {        
        printf("Voer je naam in: ");
        fgets(player.name, sizeof(player.name), stdin);
        player.name[strcspn(player.name, "\n")] = '\0';
        printf("Welkom %s, veel plezier in de DrakenGrot opzoek naar het Gouden EI!\n", player.name);

        player.health = 100;
        player.attack_power = 5;

        int room_count;
        printf("Hoeveel kamers wil je? (2-25): ");
        scanf("%d", &room_count);
        while(getchar() != '\n');
        
        if(room_count < 2 || room_count > 25){
            printf("Ongeldig aantal. Standaardwaarde 10 wordt gebruikt.\n");
            room_count = 10;
        }
        dungeon = build_dungeon(room_count);
        player.current_room = 0;
    }

    // Monsters instellen
    baby_dragon.name = "Baby Draak";
    baby_dragon.health = BABY_DRAGON_HEALTH;
    baby_dragon.attack_power = BABY_DRAGON_DAMAGE;
    
    ancient_dragon.name = "Oude Draak";  
    ancient_dragon.health = ANCIENT_DRAGON_HEALTH;
    ancient_dragon.attack_power = ANCIENT_DRAGON_DAMAGE;

    while(!game_ended){
        Room* current_room = &dungeon->rooms[player.current_room];
        current_room->visited = 1;

        printf("Je bent in kamer %d\n", current_room->id);
        
        if(current_room->cleared){
            printf("Verbindingen: ");
            RoomLink* link = current_room->links;
            while(link != NULL){
                printf("%d ", link->connected_room->id);
                link = link->next;
            }
        }
        
        process_items(current_room);
        if(game_ended == 1){
            destroy_dungeon(dungeon);
            return 0;
        }
        
        if(!current_room->cleared && current_room->has_monster){
            switch(current_room->has_monster){
                case 1: 
                    baby_dragon.health = BABY_DRAGON_HEALTH;
                    (*fight)(&baby_dragon, current_room);
                    break;
                case 2: 
                    ancient_dragon.health = ANCIENT_DRAGON_HEALTH;
                    (*fight)(&ancient_dragon, current_room);
                    break;
            }
            if(game_ended == 1){
                break;
            }
        }

        printf("\nBeschikbare kamers: ");
        RoomLink* link = current_room->links;
        while(link != NULL){
            printf("%d ", link->connected_room->id);
            link = link->next;
        }

        char input[10];
        printf("\nKies een kamer (-1 om te stoppen, 'save' om op te slaan): ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';

        if(strcmp(input, "save") == 0) {
            save_game(dungeon, &player);
            printf("Spel opgeslagen!\n");
            continue;
        }

        int new_room = atoi(input);
        if(new_room == -1) break;

        _Bool valid = 0;
        link = current_room->links;
        while(link && !valid){
            if(link->connected_room->id == new_room){
                printf("%s verlaat kamer %d en gaat naar kamer %d\n", player.name, player.current_room, new_room);
                valid = 1;
                player.current_room = new_room;
            }
            link = link->next;
        }  
        
        if(!valid){
            printf("Ongeldige keuze!\n");
        } 
    }
    
    destroy_dungeon(dungeon);
    return 0;
}