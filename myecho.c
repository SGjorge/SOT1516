/* Jorge Símon Gil
 Técnologías
 myecho.c */
 
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

/*Intentar agrupar en un tipo enum, o struct o algo VER*/
char user[] = "USUARIO";
char home[] = "CASA";
char dir[] = "DIRECTORIO";
char pid[] = "*";
char nopt[] = "-n";

char *
getEnv(char var[])
{
	char *env;
	env = getenv(var);
	if(env == NULL){
		env = "\0";
	}
	return env;
}

char *
getPid()
{
	int pd;
	char *p = "\0";
	
	pd = getpid();
	printf("%d",pd);
	return p;
}

char *
getDir()
{
	char* cwd;
	char buff[PATH_MAX + 1];

	cwd = getcwd(buff, PATH_MAX + 1 );
	if(cwd == NULL){
		return "\0";
	}
	return cwd;
}

void
safeArg(int index,char *argv[])
{	
	char usr[] = "USER";
	char hm[] = "HOME";
	
	switch(strcmp(user,argv[index])){
	case 0:
		argv[index] = getEnv(usr);
	}
	switch(strcmp(home,argv[index])){
	case 0:
		argv[index] = getEnv(hm);
	}
	switch(strcmp(dir,argv[index])){
	case 0:
		argv[index] = getDir();
	}
	switch(strcmp(pid,argv[index])){
	case 0:
		argv[index] = getPid();
	}
	switch(strcmp(nopt,argv[index])){
	case 0:
		argv[index]  = "\0";
	}
}

void
printArgs(int argc, char *argv[])
{
	int i;
	char *opt = argv[1];
	
	for(i =1; i <argc; i++){
		safeArg(i,argv);
		printf("%s ",argv[i]);
	}
	switch(strcmp(nopt,opt)){
	case -1:
		printf("\n");
		break;
	case 1:
		printf("\n");
		break;
	}

}
 
int
main(int argc, char *argv[])
{
	printf("-------------------MYECHO-----------------\n");
	if(argc != 1){
		printArgs(argc,argv);
	}
	
	exit(EXIT_SUCCESS);
}
 
 /* gcc -c -Wall -Wshadow -g myecho.c && gcc -o myecho myecho.o*/
