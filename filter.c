#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <err.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>

void
grep(int fd,char *regular)
{
	switch(fork()){
	case -1:
		err(1, "fork failed");
		break;
	case 0:
		dup2(fd,0);
		close(fd);
		execl("/bin/grep", "grep",regular, NULL);
		err(1,"execl");
	}
}

int
orderedCommand(int filed,char *argv[])
{
	int fd[2];
	char command[1024] = "/usr/bin/";

	strcat(command,argv[2]);
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
		dup2(filed,0);
		close(fd[1]);
		argv++;
		argv++;
		execv(command,argv);
		err(1,"execl");
	default:
		close(fd[1]);
		return fd[0];
	}
}

int
openFile(char *file)
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
checkRegularFile(char *name)
{
	struct stat st;
	
	if (stat(name, &st) < 0) {
		err(1, "dir");
	}
	if((st.st_mode & S_IFMT) != S_IFREG){
		return -1;
	}
	if(access(name,R_OK) != 0){
		warn("%s",name);
		return -1;
	}
	return 0;
}

void
processFile(char *name,char *argv[])
{
	int control;
	int fd;

	control = 0;
	fd = 0;

	control = checkRegularFile(name);
	if(control != 0){
		return;
	}
	fd = openFile(name);
	fd = orderedCommand(fd,argv);
	grep(fd,argv[1]);
	close(fd);
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
processDir(char *cwd, char *argv[])
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
		processFile(file,argv);
	}
	while(wait(0) > 0){
		;
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

	cwd = NULL;

	printf("------------------------- Filter ------------------------------\n");
	if(argc < 2){
		err(1,"Wrong command line");
	}
	cwd = getDir();
	processDir(cwd,argv);
	exit(EXIT_SUCCESS);
}