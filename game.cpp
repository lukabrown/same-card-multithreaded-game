//Luka Brown Project 2

#include <iostream>
#include <cstdlib>
#include <vector>
#include <pthread.h>
#include <fstream>
#include <string>

using namespace std;

static void  InitDeck();
static void* Dealer(void*);
static void* Player(void*);
static void  PrintDeck();
static void  PrintScreen();
static void  ShuffleDeck();
static char  AssignSuit(int card);

static vector<int> deck;
static pthread_mutex_t table;
static ofstream MyFile("output.txt");
static bool game = true;
static int  wins = 0;
static int  turns = 0;
static int  whos_turn = 0;
static bool roundGoing = false;
static int  prevWin = -1;
static int  hand[] = { -1,-1,-1,-1,-1 };
static int  draw[] = { -1,-1,-1,-1,-1 };
static bool alldone = false;
static int  seed = 0;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "\nUSAGE: ./game.cpp [seed]\n");
        exit(1);
    }

    string temp = argv[1];
    for (int i = 0; temp[i] != '\0'; i++) {
        seed = seed * 10 + temp[i] - '0';
    }
    srand((unsigned int)seed);

    pthread_t* const handle = new pthread_t [5];
    pthread_mutex_init(&table, NULL);

    for (int thread = 0; thread < 5; thread++) {
        if (thread == 0)
            pthread_create(&handle[thread], NULL, Dealer, (void*)(uintptr_t)thread);
        else
            pthread_create(&handle[thread], NULL, Player, (void*)(uintptr_t)thread);
    }

    while (game) {
        if (wins > 1 && alldone)
            game = false;
    }

    for (long thread = 0; thread < 5; thread++) {
        pthread_join(handle[thread], NULL);
    }

    MyFile.close();
    delete [] handle;
    pthread_mutex_destroy(&table);
    return 0;
}

static void InitDeck() {
    deck.clear();
    for (int i = 0; i <= 52; i++) {
        deck.push_back(i);
    }
}

static void PrintDeck() {
    cout << "Deck: ";
    char suit;

    for(int i = 0; i < deck.size(); i++) {
        suit = AssignSuit(deck[i]);
        cout << suit << deck[i]%13+1 << " ";
        if (((i+1) % 20) == 0) 
            cout << "\n      ";
    }
    cout << '\n';
}

static void PrintScreen() {
    char suit;
    string win;

    for (int i = 1; i < 5; i++) {
        suit = AssignSuit(hand[i]);

        if (i % 2 == prevWin % 2)
            win = "Winner\n";
        else 
            win = "Lost\n";

        if (i == prevWin) {
            cout << "Player " << i << ": hand " << suit << hand[i]%13+1 << ',';
            suit = AssignSuit(draw[i]);
            cout << suit << draw[i]%13+1 << '\n';
        }
        else
            cout << "Player " << i << ": hand " << suit << hand[i]%13+1 << '\n';
        cout << "Player " << i << ": " << win;
    }
    PrintDeck();
    cout << '\n';
}

static void* Dealer(void* id) {
    int my_id = *((int*)&id);
    srand((unsigned int)seed);

    while(game) {
        pthread_mutex_lock(&table);
        if (whos_turn == my_id) {
            InitDeck();
            ShuffleDeck();

            turns = 0;
            roundGoing = true;
            if (wins == 0)
                whos_turn = 1;
            else
                whos_turn = 2;
            
            MyFile << "\nDealer: shuffle\n";
        }
        pthread_mutex_unlock(&table);
    }
    return NULL;
}

static void* Player(void* id) {
    int my_id = *((int*)(&id));
    int partner;
    char suit;
    srand((unsigned int)seed);

    if (my_id == 1)
        partner = 3;
    else if (my_id == 2) 
        partner = 4;
    else if (my_id == 3) 
        partner = 1;
    else if (my_id == 4) 
        partner = 2;

    while(game) {
        pthread_mutex_lock(&table);
        if (!roundGoing && whos_turn == my_id) {
            hand[my_id] = -1;
            draw[my_id] = -1;
            whos_turn++;
            if (whos_turn == 5) {
                whos_turn = 0;
                if (wins == 2) {
                    alldone = true;
                    whos_turn = 6;
                }
            }
            if (my_id != prevWin)
                MyFile << "Player " << my_id << ": exits round\n";
        } else if (whos_turn == my_id) {
            if (turns == 0) {
                hand[my_id] = deck[0];
                deck.erase(deck.begin());
                whos_turn++;

                if (whos_turn == 2 && wins == 1)
                    turns += 4;

                if (wins == 1 && whos_turn == 5)
                    whos_turn = 1;

                if (whos_turn == 5) {
                    whos_turn -= 4;
                    turns += 4;
                }
                
                suit = AssignSuit(hand[my_id]);
                MyFile << "Player " << my_id << ": draw " << suit << hand[my_id]%13+1 << '\n';
            } else {
                turns++;
                whos_turn++;

                if (whos_turn == 5) {
                    whos_turn -= 4;
                    turns += 4;
                }
                draw[my_id] = deck[0];
                deck.erase(deck.begin());
                
                suit = AssignSuit(hand[my_id]);
                MyFile << "Player " << my_id << ": hand " << suit << hand[my_id]%13+1 << '\n';
                suit = AssignSuit(draw[my_id]);
                MyFile << "Player " << my_id << ": draw " << suit << draw[my_id]%13+1 << '\n';
                suit = AssignSuit(hand[my_id]);
                MyFile << "Player " << my_id << ": hand (" << suit << hand[my_id]%13+1 << ',';
                suit = AssignSuit(draw[my_id]);
                MyFile << suit << draw[my_id]%13+1 << ") <> Player ";
                suit = AssignSuit(hand[partner]);
                MyFile << partner << ": hand " << suit << hand[partner]%13+1 << "\n";

                if (hand[my_id]%13 == hand[partner]%13 || draw[my_id]%13 == hand[partner]%13) {
                    wins++;
                    roundGoing = false;

                    if (my_id % 2 == 0) 
                        MyFile << "Player " << my_id << ": Team 2 wins\n";
                    else
                        MyFile << "Player " << my_id << ": Team 1 wins\n";
                    whos_turn = 1;
                    if (whos_turn == my_id) {
                        whos_turn = 2;
                    }

                    MyFile << "Player " << my_id << ": exits round\n";
                    prevWin = my_id;
                    PrintScreen();
                } else {
                    int num = rand()%2;
                    if (num == 0) {
                        suit = AssignSuit(hand[my_id]);
                        MyFile << "Player " << my_id << ": discard " << suit << hand[my_id]%13+1 << " at random\n";
                        hand[my_id] = draw[my_id];
                    } else {
                        suit = AssignSuit(draw[my_id]);
                        MyFile << "Player " << my_id << ": discard " << suit << draw[my_id]%13+1 << " at random\n";
                    }
                    draw[my_id] = -1;
                }
                
            }
        }
        pthread_mutex_unlock(&table);
    }
    return NULL;
}

void ShuffleDeck() {
    int swap, random;
    for (int i = 0; i <= 52; i++) {
        random = rand() % 52;
        swap = deck[i];
        deck[i] = deck[random];
        deck[random] = swap;
    }
}

static char AssignSuit(int card) {
    char suit;
    int temp = card/13;

    if (temp == 0)
        suit = 'C';
    else if (temp == 1)
        suit = 'D';
    else if (temp == 2)
        suit = 'H';
    else 
        suit = 'S';
    return suit;
}