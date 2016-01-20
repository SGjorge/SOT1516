#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>

enum{
	sizeBuffer = 1024*8,
	minNumArgs = 3,
	maxNumArgs = 5,
};

void
writeOut(int fdi, int fdo)
{
	char buffer[sizeBuffer];
	int ndi;
	int ndo;

	ndi = 1;
	ndo = 0;

	if(lseek(fdo,0,2) < 0){
		err(1,"lseek");
	}
	while(ndi != 0){
		ndi = read(fdi,buffer,sizeof(buffer));
		if(ndi < 0){
			err(1,"read");
		}
		ndo = write(fdo,buffer,ndi);
		if(ndi != ndo){
			err(1,"write");
		}
	}
}

int
executeCommandLine(char *argv[],int file)
{
	char *path;
	char *pt;
	char *ph;
	char *pth;
	int fd[2];
	int fderr;

	fderr = 0;
	path = getenv("PATH");
	pt = malloc(strlen(path));
	strcpy(pt,path);

	pth = malloc(strlen(path));
	ph = strtok(pt,":");
	fderr = open("/dev/null",O_RDWR);
	if(fderr < 0){
		err(1,"open");
	}
	if(pipe(fd) == -1){
		err(1,"pipe");
	}
	switch(fork()){
	case -1:
		err(1, "fork failed");
		break;
	case 0:
		close(fd[0]);
		dup2(file,0);
		dup2(fd[1],1);
		dup2(fderr,2);
		close(fd[1]);
		argv++;
		argv++;
		while(ph != NULL){
			strcpy(pth,ph);
			strcat(pth,"/");
			strcat(pth,argv[0]);
			execv(pth,argv);
			ph = strtok(NULL,":");
		}
		err(1,"execv");
	default:
		close(fd[1]);
		return fd[0];
	}
}

int
executeCommandFifo(char **argv)
{
	char *path;
	char *pt;
	char *ph;
	char *pth;
	int fd[2];
	int fderr;

	path = getenv("PATH");
	pt = malloc(strlen(path));
	strcpy(pt,path);
	fderr = 0;

	fderr = open("/dev/null",O_RDWR);
	if(fderr < 0){
		err(1,"open");
	}
	pth = malloc(strlen(path));
	ph = strtok(pt,":");
	if(pipe(fd) == -1){
		err(1,"pipe");
	}
	switch(fork()){
	case -1:
		err(1, "fork failed");
		break;
	case 0:
		close(fd[0]);
		dup2(fd[1],1);
		dup2(fderr,2);
		close(fd[1]);
		while(ph != NULL){
			strcpy(pth,ph);
			strcat(pth,"/");
			strcat(pth,argv[0]);
			execv(pth,argv);
			ph = strtok(NULL,":");
		}
		err(1,"execv");
	default:
		close(fd[1]);
		while(wait(0) > 0){
			;
		}
		return fd[0];
	}
}

void
getCommandFifo(FILE *fd,char **cmd)
{
	char buffer[sizeBuffer];
	char *buf;
	char *control;
	char *tok;
	int i;

	i = 0;
	buf = NULL;
	control = NULL;
	tok = NULL;

	control = fgets(buffer,sizeBuffer,fd);
	if(control == NULL){
		exit(EXIT_SUCCESS);
	}
	buf = malloc(strlen(buffer));
	buf = strncpy(buf,buffer,strlen(buffer)-1);
	tok = strtok(buf," ");
	while(i < maxNumArgs){
		cmd[i] = tok;
		i++;
		tok = strtok(NULL," ");
	}
}

FILE *
openFifo(char *path)
{
	FILE *fd;

	fd = NULL;

	fprintf(stderr, "openning fifo\n");
	fd = fopen(path, "r");
	if(fd == NULL){
		err(1, "open");
	}
	return fd;
}

int
creatOut()
{
	int fd;

	fd = 0;

	fd = creat("fifocmd.out",0660);
	if(fd < 0){
		err(1,"creat");
	}
	return fd;
}

void
createFifo(char *path)
{
	int control;

	control = 0;

	control = mkfifo(path,0664);
	if(control == -1){
		err(1,"mkfifo");
	}
}

int
main(int argc, char *argv[])
{
	FILE *fd;
	int fdo;
	int fdi;
	int fdc;
	char *cmd[maxNumArgs];

	fd = NULL;
	fdc = 0;
	fdo = 0;
	fdi = 0;

	printf("------------------------ fifocmd -------------------------\n");
	if (argc < minNumArgs){
		err(1,"Wrong Command Line");
	}
	createFifo(argv[1]);
	fdo = creatOut();
	for(;;){
		fd = openFifo(argv[1]);
		getCommandFifo(fd,cmd);
		fdc = executeCommandFifo(cmd);
		fdi = executeCommandLine(argv,fdc);
		writeOut(fdi,fdo);
		fclose(fd);
	}
	exit(EXIT_SUCCESS);
}