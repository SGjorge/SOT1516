/* Jorge Simon Gil
 Tecnologias
 Proctailtxt.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>

static int maxTxt = 20;

typedef struct Cell Cell;
struct Cell{
	char *name;
	Cell *next;
};

struct List{
	Cell *first;
	int count;
};
typedef struct List List;

int
readTxt(int fd, char *buffer)
{
	int ndxR;
	ndxR = 0;
	
	ndxR = read(fd,buffer,sizeof buffer);
	if(ndxR < 0){
		err(1,"error al leer");
	}
	return ndxR;
}

void
writeAll(int fd, char *buffer,int bytes)
{
	int ndxW;
	ndxW = 0;

	ndxW = write(fd, buffer,bytes);
	if (ndxW != bytes){
		err(1, "Error de escritura");	
	}
}

void
writeOut(int fdIn, int fdOut,int bytes)
{
	int ndxR;
	char buffer[1024*8];

	ndxR = 1;
	
	if (bytes == -1){
		while(ndxR != 0){
			ndxR = readTxt(fdIn,buffer);
			writeAll(fdOut,buffer,ndxR);
		}
		return;
	}
	ndxR = readTxt(fdIn,buffer);
	while(ndxR < bytes){
		writeAll(fdOut,buffer,ndxR);
		bytes = bytes - ndxR;
		ndxR = readTxt(fdIn,buffer);
	}
	writeAll(fdOut,buffer,bytes);
}

void
fileName(char *file)
{
	char *newFile;

	newFile = file;

	newFile = strcat(file,".out");
	if(newFile == NULL){
		err(1,"strcat");
	}
	fprintf(stdout,"%s\n",newFile);
}

int
createOut(char *name)
{
	int fd;

	fd = 0;

	fileName(name);
	fd = creat(name,0660);
	if(fd == -1){
		err(1,"file");
	}
	return fd;
}

void
fileOut(int fd, int bytes,char *file)
{
	int offset;
	int fdW;

	offset = 1;
	fdW = 0;

	switch(bytes){
	case -1:
		fdW = createOut(file);
		writeOut(fd,fdW,bytes);
		break;
	default:
		offset = lseek(fd,-bytes,2);
		if(offset == -1){
			fileOut(fd,offset,file);
		}else{
			fdW = createOut(file);
			writeOut(fd,fdW,bytes);
		}
		break;
	}
}

int
openTxt(char *file)
{	
	int fd;
	fd = 0;
	
	fd = open(file,O_RDONLY);
	if(fd < 0){
		err(1,"file");
	}
	return fd;
}

void
createSon(char *file,int bytes)
{
	int fd;
	fd = 0;

	switch(fork()){
	case -1:
		err(1, "fork failed");
		break;
	case 0:
		fd = openTxt(file);
		fileOut(fd,bytes,file);
		close(fd);
		exit(EXIT_SUCCESS);
	}
}

void
processTxt(List *list, int bytes)
{
	Cell *aux = malloc(sizeof(Cell));

	aux = (*list).first;

	if((*list).count > maxTxt){
		err(1,"Txt files number exceeded");
	}
	while(aux != NULL){
		createSon((*aux).name,bytes);
		aux = (*aux).next;
	}
	while(wait(0) != -1){
		;
	}
}

int
checkFile(char *name)
{
	struct stat st;
	
	if (stat(name, &st) < 0) {
		err(1, "dir");
	}
	if((st.st_mode & S_IFMT) != S_IFREG){
		return -1;
	}
	if(access(name,R_OK) != 0){
		return -1;
	}
	return 0;
}

int
checkTxt(char *name)
{
	char *txt;
	int control;
	
	txt = NULL;
	control = 0;
	
	txt = strstr(name,".txt");
	if(txt == NULL){
		return -1;
	}
	if(strcmp(txt,".txt\0") != 0){
		control = -1;
	}
	return control;
}

void
addList(List *list, char *name)
{
	Cell *cell = malloc(sizeof(Cell));
	(*cell).name = strdup(name);
	(*cell).next = NULL;

	Cell *aux = malloc(sizeof(Cell));


	if ((*list).first == NULL){
		(*list).first = malloc(sizeof(Cell));
		memcpy((*list).first,cell,sizeof(Cell));
		(*list).count ++;
		return;
	}
	aux = (*list).first;
	while((*aux).next != NULL){
		aux = (*aux).next;
	}
	(*aux).next = malloc(sizeof(Cell));
	memcpy((*aux).next,cell,sizeof(Cell));
	(*list).count ++;
}

void
saveNameFile(char * file, List *list)
{
	int control;

	control = 0;
	
	control = checkTxt(file);
	if(control != 0){
		return;
	}
	control = checkFile(file);
	if(control != 0){
		warn("No se pudo leer %s satisfactoriamente",file);
		return;
	}
	addList(list,file);
}

DIR *
openDir(char *dir)
{
	DIR *dd;
	dd = NULL;
	
	dd = opendir(dir);
	if(dd == NULL){
		err(1,"dir");
	}
	return dd;
}

char *
readDir(DIR * dir)
{
	struct dirent *dr;
	dr = NULL;
	
	dr = readdir(dir);
	if(dr == NULL){
		return NULL;
	}
	return (*dr).d_name;
}

void
processDir(char *cwd,List *list)
{
	DIR *dir;
	char *file;
	
	dir = NULL;
	file = NULL;
	
	dir = openDir(cwd);
	for(;;){
		file = readDir(dir);
		if(file == NULL){
			break;
		}
		saveNameFile(file,list);
	}

}

char *
getDir()
{
	char* cwd;
	char buff[PATH_MAX + 1];

	cwd = getcwd(buff, sizeof(buff));
	if(cwd == NULL){
		err(1,"cwd");
	}
	return cwd;
}

int
main(int argc, char *argv[])
{
	char *cwd;
	int bytes;
	List *list = malloc(sizeof(List));

	(*list).first = NULL;
	(*list).count = 0;
	cwd = NULL;
	bytes = -1;

	printf("-------------------- Proctailtxt ------------------\n");
	cwd = getDir();
	switch(argc){
	case 1:
		processDir(cwd,list);
		processTxt(list,bytes);
		break;
	case 2:
		bytes = atoi(argv[1]);
		processDir(cwd,list);
		processTxt(list,bytes);
		break;
	default:
		err(1,"wrong command line");
		break;
	}

	exit(EXIT_SUCCESS);
}