#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

#define MAX 100
int i;
// -------- SONG --------
typedef struct Song {
    char title[50];
    char artist[50];
    float duration;
    char genre[30];
    struct Song *next, *prev;
} Song;

// -------- DATABASE --------
sqlite3 *db;
int current_user_id = -1;

// -------- DS --------
Song* queue[MAX];
int front = 0, rear = -1;

Song* stack[MAX];
int top = -1;

Song* head = NULL;
Song* tail = NULL;
Song* current = NULL;

// -------- TREE --------
typedef struct GenreNode {
    char genre[30];
    Song* songs[MAX];
    int count;
    struct GenreNode *left, *right;
} GenreNode;

GenreNode* root = NULL;

// -------- DB INIT --------
void initDB() {
    sqlite3_open("playlist.db", &db);

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY, username TEXT, password TEXT);",
        0,0,0);

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS songs(id INTEGER PRIMARY KEY, user_id INT, title TEXT, artist TEXT, duration REAL, genre TEXT);",
        0,0,0);
}

// -------- REGISTER --------
void registerUser() {
    char u[50], p[50];

    printf("Username: ");
    scanf("%s", u);

    printf("Password: ");
    scanf("%s", p);

    char sql[200];
    sprintf(sql,"INSERT INTO users(username,password) VALUES('%s','%s');",u,p);

    sqlite3_exec(db,sql,0,0,0);
    printf("Registered!\n");
}

// -------- LOGIN --------
int loginUser() {
    char u[50], p[50];

    printf("Username: ");
    scanf("%s", u);

    printf("Password: ");
    scanf("%s", p);

    char sql[200];
    sprintf(sql,"SELECT id FROM users WHERE username='%s' AND password='%s';",u,p);

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,sql,-1,&stmt,0);

    if(sqlite3_step(stmt)==SQLITE_ROW) {
        current_user_id = sqlite3_column_int(stmt,0);
        printf("Login successful!\n");
        return 1;
    }

    printf("Invalid login!\n");
    return 0;
}

// -------- SAVE SONG --------
void saveSong(Song* s) {
    char sql[300];
    sprintf(sql,
        "INSERT INTO songs(user_id,title,artist,duration,genre) VALUES(%d,'%s','%s',%f,'%s');",
        current_user_id,s->title,s->artist,s->duration,s->genre);

    sqlite3_exec(db,sql,0,0,0);
}

// -------- TREE --------
GenreNode* createNode(char* genre, Song* s) {
    GenreNode* node = (GenreNode*)malloc(sizeof(GenreNode));
    strcpy(node->genre, genre);
    node->songs[0] = s;
    node->count = 1;
    node->left = node->right = NULL;
    return node;
}

GenreNode* insertTree(GenreNode* root, Song* s) {
    if(root == NULL) return createNode(s->genre, s);

    int cmp = strcmp(s->genre, root->genre);

    if(cmp == 0)
        root->songs[root->count++] = s;
    else if(cmp < 0)
        root->left = insertTree(root->left, s);
    else
        root->right = insertTree(root->right, s);

    return root;
}

void inorder(GenreNode* root) {
    if(root) {
        inorder(root->left);

        printf("\nGenre: %s\n", root->genre);
        for(i=0;i<root->count;i++)
            printf(" - %s\n", root->songs[i]->title);

        inorder(root->right);
    }
}

// -------- ADD SONG --------
void addSong() {

    Song* s = (Song*)malloc(sizeof(Song));

    printf("Title: ");
    scanf("%s", s->title);

    printf("Artist: ");
    scanf("%s", s->artist);

    printf("Duration: ");
    scanf("%f", &s->duration);

    printf("Genre: ");
    scanf("%s", s->genre);

    s->next = s->prev = NULL;

    // Queue
    queue[++rear] = s;

    // Linked List
    if(head == NULL) head = tail = s;
    else {
        tail->next = s;
        s->prev = tail;
        tail = s;
    }

    // Tree
    root = insertTree(root,s);

    // Save DB
    saveSong(s);

    printf("Song added!\n");
}

// -------- LOAD SONGS --------
void loadSongs() {

    char sql[200];
    sprintf(sql,"SELECT title,artist,duration,genre FROM songs WHERE user_id=%d;",current_user_id);

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,sql,-1,&stmt,0);

    while(sqlite3_step(stmt)==SQLITE_ROW) {

        Song* s = (Song*)malloc(sizeof(Song));

        strcpy(s->title,(char*)sqlite3_column_text(stmt,0));
        strcpy(s->artist,(char*)sqlite3_column_text(stmt,1));
        s->duration = sqlite3_column_double(stmt,2);
        strcpy(s->genre,(char*)sqlite3_column_text(stmt,3));

        s->next = s->prev = NULL;

        queue[++rear] = s;

        if(head == NULL) head = tail = s;
        else {
            tail->next = s;
            s->prev = tail;
            tail = s;
        }

        root = insertTree(root,s);
    }
}

// -------- DISPLAY --------
void displayQueue() {
    if(front > rear) {
        printf("Empty playlist!\n");
        return;
    }

    for(i=front;i<=rear;i++) {
        printf("%d. %s - %s\n", i+1, queue[i]->title, queue[i]->artist);
    }
}

// -------- PLAYER --------
void playFirst() {
    if(front > rear) return;

    current = queue[front];
    printf("Playing: %s\n", current->title);
    stack[++top] = current;
}

void nextSong() {
    if(current && current->next) {
        current = current->next;
        printf("Playing: %s\n", current->title);
        stack[++top] = current;
    } else {
        printf("No next song!\n");
    }
}

void prevSong() {
    if(current && current->prev) {
        current = current->prev;
        printf("Playing: %s\n", current->title);
    } else {
        printf("No previous song!\n");
    }
}

// -------- HISTORY --------
void showHistory() {
    if(top == -1) {
        printf("No history!\n");
        return;
    }

    for(i=top;i>=0;i--) {
        printf("%s\n", stack[i]->title);
    }
}

// -------- MENU --------
void menu() {
    int ch;

    do {
        printf("\n1.Add\n2.Display\n3.Play\n4.Next\n5.Prev\n6.History\n7.Genre\n8.Exit\nChoice: ");
        scanf("%d",&ch);

        switch(ch) {
            case 1: addSong(); break;
            case 2: displayQueue(); break;
            case 3: playFirst(); break;
            case 4: nextSong(); break;
            case 5: prevSong(); break;
            case 6: showHistory(); break;
            case 7: inorder(root); break;
        }

    } while(ch != 8);
}

// -------- MAIN --------
int main() {

    initDB();

    int ch;

    do {
        printf("\n1.Register\n2.Login\nChoice: ");
        scanf("%d",&ch);

        if(ch == 1) registerUser();

        else if(ch == 2) {
            if(loginUser()) {
                loadSongs();
                menu();
                break;
            }
        }

    } while(1);

    return 0;
}
