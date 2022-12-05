//Luka Brown 

//Simple Card Game
//Runs 4 player threads and a dealer thread
//Dealer shuffles, then players draw
//then players rotate turns drawing and discarding
//until their partner has a matching card which is a win
//master thread waits for 2 games to finish before joining

#include <iostream>
#include <cstdlib>
#include <vector>
#include <pthread.h>
#include <fstream>
#include <string>
using namespace std;

//function prototypes
static void  InitDeck();
static void* Dealer(void*);
static void* Player(void*);
static void  PrintDeck();
static void  PrintScreen();
static void  ShuffleDeck();
static char  AssignSuit(int card);

//global variables
static vector<int> deck;
static pthread_mutex_t table;
static ofstream MyFile("output.txt");
static bool game, roundGoing, alldone;
static int  wins, turns, whos_turn, seed, prevWin;
static int  hand[] = { -1,-1,-1,-1,-1 };
static int  draw[] = { -1,-1,-1,-1,-1 };

//master thread checks game status after creating game
int main(int argc, char *argv[]) {
    //parameter checking
    if (argc != 2) {
        fprintf(stderr, "\nUSAGE: ./game.cpp [seed]\n");
        exit(1);
    }

    //grabs and sets seed
    string temp = argv[1];
    for (int i = 0; temp[i] != '\0'; i++) {
        seed = seed * 10 + temp[i] - '0';
    }
    srand((unsigned int)seed);

    //sets globals
    wins = turns = whos_turn = 0;
    game = true;
    roundGoing = alldone = false;
    prevWin = -1;

    //pthreads variables
    pthread_t* const handle = new pthread_t [5];
    pthread_mutex_init(&table, NULL);

    //creating threads
    //0 dealer, 1-4 player
    for (int thread = 0; thread < 5; thread++) {
      if (thread == 0)
       pthread_create(&handle[thread], NULL, Dealer, (void*)(uintptr_t)thread);
      else
       pthread_create(&handle[thread], NULL, Player, (void*)(uintptr_t)thread);
    }

    //master thread checks if game has finished 2 rounds and all threads exited
    while (game) {
        if (wins > 1 && alldone)
            game = false;
    }

    //joins threads
    for (int thread = 0; thread < 5; thread++) {
        pthread_join(handle[thread], NULL);
    }

    //cleanup
    MyFile.close();
    delete [] handle;
    pthread_mutex_destroy(&table);
    return 0;
}
///////////////////////////////////////////////////////////////////////////////

//creates a deck of 52 unique cards
static void InitDeck() {
    deck.clear();
    for (int i = 1; i <= 52; i++) {
        deck.push_back(i);
    }
}
///////////////////////////////////////////////////////////////////////////////

//prints remaining deck to screen
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
///////////////////////////////////////////////////////////////////////////////

//outputs game results to screen
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
            cout << "Player " << i << ": hand "<< suit << hand[i]%13+1 << '\n';
        cout << "Player " << i << ": " << win;
    }
    PrintDeck();
    cout << '\n';
}
///////////////////////////////////////////////////////////////////////////////

//dealer thread. grabs mutex, check if it is dealer's turn, else unlock mutex
static void* Dealer(void* id) {
    int my_id = *((int*)&id);
    srand((unsigned int)seed);

    while(game) {
        pthread_mutex_lock(&table);
        if (whos_turn == my_id) {
            //reset and shuffle deck
            InitDeck();
            ShuffleDeck();
            
            //sets vars so other threads know to draw
            turns = 0;
            roundGoing = true;

            //starting player rotates on 2nd round
            if (wins == 0)
                whos_turn = 1;
            else
                whos_turn = 2;
            
            MyFile << "\nDealer: shuffle\n";
        } //end if turn
        pthread_mutex_unlock(&table);
    } //end while game
    return NULL;
}
///////////////////////////////////////////////////////////////////////////////

//player thread. grabs mutex, check if it is players' turn, else unlock mutex
static void* Player(void* id) {
    int my_id = *((int*)(&id));
    int partner;
    char suit;
    srand((unsigned int)seed);

    //assigns partner, to player across from me
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
        if (!roundGoing && whos_turn == my_id) { //if end phase
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
        } else if (whos_turn == my_id) { //if my turn
            if (turns == 0) { //if draw phase
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
                MyFile << "Player " << my_id << ": draw " << suit;
                MyFile << hand[my_id]%13+1 << '\n';
            } else { //regular turn
                turns++;
                whos_turn++;

                if (whos_turn == 5)
                    whos_turn -= 4;

                draw[my_id] = deck[0];
                deck.erase(deck.begin());
                
                suit = AssignSuit(hand[my_id]);
                MyFile << "Player " << my_id << ": hand ";
                MyFile << suit << hand[my_id]%13+1 << '\n';

                suit = AssignSuit(draw[my_id]);
                MyFile << "Player " << my_id << ": draw ";
                MyFile << suit << draw[my_id]%13+1 << '\n';

                suit = AssignSuit(hand[my_id]);
                MyFile << "Player " << my_id << ": hand (";
                MyFile << suit << hand[my_id]%13+1 << ',';

                suit = AssignSuit(draw[my_id]);
                MyFile << suit << draw[my_id]%13+1 << ") <> Player ";

                suit = AssignSuit(hand[partner]);
                MyFile << partner << ": hand " << suit;
                MyFile << hand[partner]%13+1 << "\n";

                //if win detected
                if (hand[my_id]%13 == hand[partner]%13 || draw[my_id]%13 == hand[partner]%13) {
                    wins++;
                    roundGoing = false;
                    whos_turn = 1;
                    prevWin = my_id;

                    if (whos_turn == my_id)
                        whos_turn = 2;

                    if (my_id % 2 == 0) 
                        MyFile << "Player " << my_id << ": Team 2 wins\n";
                    else
                        MyFile << "Player " << my_id << ": Team 1 wins\n";

                    MyFile << "Player " << my_id << ": exits round\n";
                    PrintScreen();
                } else { //if no win then randomly discard
                    int num = rand()%2;
                    if (num == 0) {
                        suit = AssignSuit(hand[my_id]);
                        MyFile << "Player " << my_id << ": discard ";
                        MyFile << suit << hand[my_id]%13+1 << " at random\n";
                        hand[my_id] = draw[my_id];
                    } else {
                        suit = AssignSuit(draw[my_id]);
                        MyFile << "Player " << my_id << ": discard " << suit;
                        MyFile << draw[my_id]%13+1 << " at random\n";
                    }
                    draw[my_id] = -1;
                } //end discard
            } //end regular turn
        } //end if turn
        pthread_mutex_unlock(&table);
    } //end while game
    return NULL;
}
///////////////////////////////////////////////////////////////////////////////

//randomly shuffles deck
static void ShuffleDeck() {
    int swap, random;
    for (int i = 0; i < 52; i++) {
        random = rand() % 52;
        swap = deck[i];
        deck[i] = deck[random];
        deck[random] = swap;
    }
}
///////////////////////////////////////////////////////////////////////////////

//helper function that returns char of which suit the card is
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
///////////////////////////////////////////////////////////////////////////////