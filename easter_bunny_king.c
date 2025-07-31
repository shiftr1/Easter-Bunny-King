#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>


typedef struct {
    char name[100];
    char poem[256];
    int  eggs;
} Bunny;

Bunny stable[100];
int   n_bunnies = 0;



void strip_newline(char *s) 
{
     s[strcspn(s, "\n")] = '\0'; 
}



void ask(const char *prompt, char *buf, int len)
{
    printf("%s", prompt);
    if (!fgets(buf, len, stdin)) buf[0] = '\0';
    strip_newline(buf);
}


void clear_stdin(void) 
{ 
    int c; while ((c = getchar()) != '\n' && c != EOF); 
}


void load_from_file(void)
{
    FILE *f = fopen("bunnies.txt", "r");
    if (!f) return;                                  


    while (n_bunnies < 100 &&
           fscanf(f, "%[^|]|%[^|]|%d\n",
                  stable[n_bunnies].name,
                  stable[n_bunnies].poem,
                 &stable[n_bunnies].eggs) == 3)
        ++n_bunnies;

    fclose(f);
}


void save_to_file(void)
{
    FILE *f = fopen("bunnies.txt", "w");
    if (!f) { perror("bunnies.txt"); return; }

    for (int i = 0; i < n_bunnies; ++i)
        fprintf(f, "%s|%s|%d\n",
                stable[i].name, stable[i].poem, stable[i].eggs);

    fclose(f);
}



void menu_register(void)
{
    if (n_bunnies == 100) { puts("Stable is full"); return; }

    Bunny b = { .eggs = 0 };
    puts("\n Register a new bunny ");
    ask("Name : ", b.name,  100);
    ask("Poem : ", b.poem,  256);

    stable[n_bunnies++] = b;
    save_to_file();

}

void menu_list(void)
{
    puts("\n Registered bunnies ");
    if (!n_bunnies) { puts("(none)\n"); return; }

    for (int i = 0; i < n_bunnies; ++i)
        printf("%2d. %s  (eggs: %d)\n    \"%s\"\n",
               i + 1, stable[i].name, stable[i].eggs, stable[i].poem);
    putchar('\n');
}

void menu_modify(void)
{
    menu_list();
    printf("Number to modify: "); int idx;
    if (scanf("%d", &idx) != 1) { clear_stdin(); return; }
    clear_stdin();
    if (idx < 1 || idx > n_bunnies) { puts("Invalid number."); return; }
    --idx;                                           

    ask("New name : ", stable[idx].name, 100);
    ask("New poem : ", stable[idx].poem, 256);
    save_to_file();

}

void menu_delete(void)
{
    menu_list();
    printf("Number to delete: "); int idx;
    if (scanf("%d", &idx) != 1) { clear_stdin(); return; }
    clear_stdin();
    if (idx < 1 || idx > n_bunnies) { puts("Invalid number"); return; }

    for (int i = idx - 1; i < n_bunnies - 1; ++i) stable[i] = stable[i + 1];
    --n_bunnies;
    save_to_file();

}


void announce_arrival(int sig) 
{ 
    (void)sig; puts("Chief Bunny: bunny arrived"); 
}


void menu_watering(void)
{
    if (!n_bunnies) { puts("No bunnies registered"); return; }

    int pipefd[2];
    if (pipe(pipefd) == -1) { perror("pipe"); return; }

   
    struct sigaction sa = { .sa_handler = announce_arrival };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction"); close(pipefd[0]); close(pipefd[1]); return;
    }

    pid_t child = fork();
    if (child == -1) 
    { 
        perror("fork"); return; 
    }

    if (child == 0) {
        close(pipefd[0]);                          
        srand(time(NULL) ^ getpid());

        for (int i = 0; i < n_bunnies; ++i) {
            kill(getppid(), SIGUSR1);              
            sleep(1);

            printf("\n%s recites:\n\"%s\"\n", stable[i].name, stable[i].poem);

            int eggs = rand() % 20 + 1;
            printf("%s received %d red eggs!\n", stable[i].name, eggs);

            if (write(pipefd[1], &eggs, sizeof eggs) != sizeof eggs)
                perror("write");
        }
        close(pipefd[1]);
        _exit(0);
    }

   
    close(pipefd[1]);                              

    for (int i = 0; i < n_bunnies; ++i) {
        int eggs = 0;
        ssize_t r;
        while ((r = read(pipefd[0], &eggs, sizeof eggs)) == -1 && errno == EINTR)
            ;                                     
        if (r != sizeof eggs) { perror("read"); eggs = 0; }
        stable[i].eggs = eggs;
    }
    close(pipefd[0]);
    waitpid(child, NULL, 0);                       

    save_to_file();

    

    int king = 0;
    for (int i = 1; i < n_bunnies; ++i)
        if (stable[i].eggs > stable[king].eggs) king = i;

   
    puts("\n  THE  EASTER  BUNNY  KING  \n");
    printf("Winner : %s\nEggs   : %d\n",
           stable[king].name, stable[king].eggs);
}


void show_menu(void)
{
    for (;;) {
        puts("\n Easter Bunny King Menu ");
        puts("1) Register bunny");
        puts("2) List bunnies");
        puts("3) Modify bunny");
        puts("4) Delete bunny");
        puts("5) Start watering and election");
        puts("0) Exit");
        printf("Choice: ");

        int pick;
        if (scanf("%d", &pick) != 1) { clear_stdin(); continue; }
        clear_stdin();

        switch (pick) {
            case 1: menu_register(); break;
            case 2: menu_list();     break;
            case 3: menu_modify();   break;
            case 4: menu_delete();   break;
            case 5: menu_watering(); break;
            case 0: return;
            default: puts("wrong choice"); break;
        }
    }
}


int main(void)
{
    load_from_file();
    show_menu();
    return 0;
}
