/* Jorge Simon Gil
 Tecnologias
 Tailtxt.c */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>

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

int
readTxt(int fd, char *buffer)
{
	int ndxR;
	ndxR = 0;
	
	ndxR = read(fd,buffer,sizeof buffer);
	if(ndxR < 0){
		err(1,"error al leer");
		exit(EXIT_FAILURE);
	}
	return ndxR;
}

void
printTxt(char *buffer,int bytes)
{
	int ndxW;
	ndxW = 0;
	
	ndxW = write(0, buffer, bytes);
	if (ndxW != bytes){
		err(1, "Error de escritura");
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
	if(strcmp(txt,".txt") != 0){
		control = -1;
	}
	return control;
}

void
printAllTxt(int fd)
{
	char buffer[8 * 1024];
	int offset;
	
	offset = 1;
	
	while(offset){
		offset = readTxt(fd,buffer);
		printTxt(buffer,offset);
	}
}

void
printLastBytes(int fd,int bytes)
{
	char buffer[8 * 1024];
	int offset;
	int lastBytes;
	
	offset = 1;
	lastBytes = bytes;
	
	while(lastBytes){
		offset = readTxt(fd,buffer);
		lastBytes = lastBytes - offset;
		if(lastBytes == 0){
			printTxt(buffer,offset);
		}else if (lastBytes < 0){
			printTxt(buffer,bytes);
			lastBytes  = 0;
		}else{
			printTxt(buffer,offset);
			printLastBytes(fd,lastBytes);
			lastBytes  = 0;
		}
	}
}

void
reader(int fd, int bytes)
{
	int offset;

	offset = 1;
	
	switch(bytes){
	case -1:
		printAllTxt(fd);
		break;
	default:
		offset = lseek(fd,-bytes,2);
		if(offset == -1){
			reader(fd,offset);
		}else{
			printLastBytes(fd,bytes);
		}
		break;
	}
	
}

void
processFile(char * file, int bytes)
{
	int control;
	int fd;

	control = 0;
	fd = 0;
	
	control = checkTxt(file);
	if(control != 0){
		return;
	}
	control = checkFile(file);
	if(control != 0){
		warn("No se pudo leer todos los .txt satisfactoriamente");
		return;
	}
	fd = openTxt(file);
	reader(fd,bytes);
	close(fd);
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
processDir(char *cwd,int bytes)
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
		processFile(file,bytes);
	}
}

int
main(int argc, char *argv[])
{
	char *cwd;
	int bytes;
	
	cwd = NULL;
	bytes = -1;
	
	printf("--------------------- TailTxt------------------\n");
	cwd = getDir();
	switch(argc){
	case 1:
		processDir(cwd,bytes);
		break;
	case 2:
		bytes = atoi(argv[1]);
		processDir(cwd,bytes);
		break;
	default:
		err(1,"wrong command line");
		break;
	}
	
	exit(EXIT_SUCCESS);
}