#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <malloc.h>
#define True 1
#define GREEN "\x1b[32m"
#define RED "\x1b[31m"
#define BLUE "\x1b[36m"
#define MAGENTA "\e[0;35m"
#define YELLOW "\x1b[33m"
#define reset "\e[0m"
char* lastcwd;

ssize_t odczytywanie(char* tekst, size_t size)
{
    ssize_t count;
    count = getline(&tekst, &size, stdin);

    return count;
}

void komendaHelp()
{
    printf("[MICROSHELL] Informacje o projekcie:\n Autor: Marta Torzynska (481887)\n Obslugiwane komendy: \n \
    help - wyswietla informacje o projekcie\n \
    exit - konczy dzialanie programu\n \
    cd <argument> - zmienia aktualny katalog roboczy\n \
    \t .. - zmienia katalog na nadrzedny\n \
    \t ~ - zmienia katalog na katalog domowy\n \
    \t - - zmienia katalog na poprzedni katalog\n \
    \t brak argumentu skutkuje zmiana katalogu na katalog domowy\n \
    ln <plik1> <plik2> - tworzy dowiazanie twarde pliku2 do pliku1\n \
    \t -s - tworzy dowiazanie symboliczne\n \
    tree - wyswietla zawartosc aktualnego katalogu w postaci drzewa\n \
    cp <plik1> <plik2> - kopiuje zawartosc pliku1 do pliku2\n");
}

void dzielenienaArgumenty(char* input, char** tablica)
{
    char* argument;
    argument = strtok(input, " ");
    strcpy(tablica[0], argument);
    int i=1;
    while(argument != NULL)
    {
        argument = strtok(NULL, " ");
        tablica[i] = argument;
        i++;
    }
}

int komendaCd(char** tablica, char* cwd)
{
    if(tablica[1] == NULL) tablica[2] = NULL;
    char* dohome = getenv("HOME");
    char* dozmiany = tablica[1];
    if(tablica[2] != NULL)
    {
        printf("ERROR: Too many arguments.\n");
        return -1;
    }

    if(tablica[1] != NULL && strcmp(tablica[1], "-") == 0)
    {
        int code = chdir(lastcwd);
        strcpy(lastcwd, cwd);
        return code;
    }
    strcpy(lastcwd, cwd);

    if(tablica[1] == NULL)
    {
        return chdir(dohome);
    }
    if(strcmp(tablica[1], "..") == 0)
    {
        return chdir("..");
    }
    if(strcmp(tablica[1], "~") == 0)
    {
        return chdir(dohome);
    }

    return chdir(dozmiany);
}

int komendaCp(char **argument)
{
    if(argument[1] == NULL || argument[2] == NULL)
    {
        printf("ERROR: Argument cannot be null\n");
        return -1;
    }
    if(argument[3] != NULL)
    {
        printf("ERROR: Too many arguments\n");
        return -1;
    }
    int fd_from = open(argument[1], O_RDONLY);
    if(fd_from == -1)
    {
       perror("ERROR");
       close(fd_from);
       return -1;
    } 
    int fd_to = open(argument[2], O_CREAT | O_RDWR, 0666);
    if(fd_to == -1)
    {
        perror("ERROR");
        close(fd_from);
        return -1;
    }

    int buffersize = 256;
    char* buffer = (char*)malloc(buffersize);
    int odczyt;

    while((odczyt = read(fd_from, buffer, buffersize)), odczyt > 0)
    {
        write(fd_to, buffer, odczyt);
    }

    close(fd_from);
    close(fd_to);
    free(buffer);
    return 0;
}

int poleceniazPATH(char** argument)
{
    pid_t childpid = fork();
    if(childpid == 0)
    {//kod dziecka
        if (execvp(argument[0], argument) < 0)
        {
        printf("ERROR: Command not found\n");
        }
        exit(errno);
    }
    else
    {//kod rodzica
        int status;
        if(waitpid(childpid, &status, 0) == -1)
        {
            errno = status;
            perror("ERROR[fork]");
        }
    }
    return 0;
}
int komendaLn(char** argument)
{
    if(argument[1] == NULL|| argument[2]==NULL)
    {
        printf("ERROR: Arguments cannot be null\n");
        return -1;
    }
    if(strcmp(argument[1], "-s") == 0)
    {
        if(argument[3] == NULL)
        {
            printf("ERROR: Arguments cannot be null\n");
            return -1;
        }
        if(argument[4] != NULL)
        {
            printf("ERROR: Too many arguments\n");
            return -1;
        }
        if(symlink(argument[2], argument[3]) != 0)
        {
            perror("ERROR");
            return -1;
        }
    }
    else 
    {
        if(argument[3] != NULL)
        {
            printf("ERROR: Too many arguments\n");
            return -1;
        } 
    
        if(link(argument[1], argument[2]) != 0)
        {
            perror("ERROR");
            return -1;
        }
    }
    return 0;
}

int komendaTree(char* cwd, int level)
{
    DIR * dir = opendir(cwd);
    if((readdir(dir)) == NULL)
            {
                closedir(dir);
                return 0;
            }
    struct dirent *katalog;
    char* namecopy = (char*)malloc(2*BUFSIZ*sizeof(char));
    char* copyofcwd = (char*)malloc(2*BUFSIZ*sizeof(char));
    char* nazwa = (char*)malloc(2*BUFSIZ*sizeof(char));
    char* wciecia = (char*)malloc(128*sizeof(char));
    memset(wciecia, '\0', 128);
    memset(wciecia, ' ', level*3);
    if(dir == NULL) 
    {
        perror("ERROR");
        return -1;
    }
    while((katalog = readdir(dir)) != NULL)
    {
        if(strcmp(katalog->d_name,".") == 0 || strcmp(katalog->d_name, "..")==0)
        {
            continue;
        }
        if(katalog->d_type != DT_DIR)
        {
            printf("%s", wciecia);
            if(katalog->d_type == DT_LNK)
            {
            printf("|__ "YELLOW"%s\n"reset, katalog->d_name);
            }
            else
            {
            printf("|__ %s\n", katalog->d_name);
            }
        }
        else
        {
            printf("%s", wciecia);
            printf("|__ "MAGENTA"%s\n"reset, katalog->d_name);
            namecopy = strcpy(namecopy,"/");
            nazwa = strcpy(nazwa, katalog->d_name);
            strcat(namecopy, katalog->d_name);
            strcpy(copyofcwd, cwd);
            strcat(copyofcwd, namecopy);
            DIR * nextdir = opendir(copyofcwd);
            if(nextdir == NULL)
            {
                perror("ERROR");
                closedir(nextdir);
                return -1;
            }
            closedir(nextdir);
            komendaTree(copyofcwd, level+1);
        }
    }
    closedir(dir);
    free(namecopy);
    free(copyofcwd);
    free(wciecia);
    free(nazwa);
    return 0;
}

int main(int argc, char *argv[])
{
    lastcwd = (char*)malloc(BUFSIZ);
    char* cwd = (char*)malloc(BUFSIZ*sizeof(char));
    char* tekst = (char *)malloc(BUFSIZ * sizeof(char));
    char** argument = (char**) malloc(10*sizeof(char*));
    for(int i=0;i<10;i++)
    {
        argument[i] = (char*) malloc(BUFSIZ*sizeof(char));
    }

    char* argument_copy[10];
    for(int i = 0; i < 10; i++)
    {
        argument_copy[i] = argument[i];
    }

    strcpy(lastcwd, "/");
    while (True)
    {
        cwd = getcwd(cwd, BUFSIZ);
        printf(BLUE"[%s" GREEN":%s]$\n"reset,getenv("LOGNAME"), cwd); //znak zachety

        ssize_t count = odczytywanie(tekst, BUFSIZ);
        tekst[strcspn ( tekst, "\n" )] = '\0'; //usuwa \n z getline

        if(count == 1 || count == 0)
        {
            continue;
        }
        if (count == -1)
        {
            perror("ERROR");
            continue;
        }
        
        dzielenienaArgumenty(tekst, argument);
        if(strcmp(argument[0], "help") == 0)
        {
            komendaHelp();
            continue;
        }
        if(strcmp(argument[0], "exit") == 0)
        {
            break;
        }

        if(strcmp(argument[0], "cd") == 0)
        {
            if(komendaCd(argument, cwd) == -1) perror("ERROR");
            continue;
        }
        if(strcmp(argument[0], "cp") == 0)
        {
            komendaCp(argument);
            continue;
        }
        if(strcmp(argument[0], "ln") == 0)
        {
            komendaLn(argument);
            continue;
        }
        if(strcmp(argument[0], "tree") == 0)
        {
            komendaTree(cwd, 0);
            continue;
        }
        if(poleceniazPATH(argument) != 0)
        {
            continue;
        }

    }

    for(int i=0;i<10;i++)
    {
        free(argument_copy[i]);
    }

    free(argument);
    free(tekst);
    free(cwd);
    free(lastcwd);
	return 0;
}