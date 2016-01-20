#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>

static int statexit;
static int flagTime;

typedef struct Cell Cell;
struct Cell
{
	char *name;
	Cell *next;
};

struct List
{
	Cell *first;
};
typedef struct List List;

struct Extensions
{
	char *tst;
	char *ok;
	char *out;
	char *cond;
};
typedef struct Extensions Extensions;
Extensions ext = {".tst\0",".ok\0",".out\0",".cond\0"};

struct Options
{
	char *c;
	char *t;
};
typedef struct Options Options;
Options opt = {"-c\0","-t\0"};

char *
putoffExt(char *file, char *exten)
{
	char *f;
	char *nf;

	f = NULL;
	nf = NULL;

	f = malloc(sizeof(file));
	f = strstr(file,exten);
	nf = malloc(strlen(file) - strlen(f));
	nf = strndup(file,(strlen(file) - strlen(f)));
	return strdup(nf);
}

void
executeCommand(char *pt, char *cmd, char *argv)
{
	char *wd;

	wd = NULL;

	while(pt != NULL){
		wd = strdup(pt);
		strcat(wd,"/");
		strcat(wd,cmd);
		execl(wd,cmd,argv,NULL);
		pt = strtok(NULL,":");
	}
	err(1,"execl");
}

void
executeChdir(char *dir)
{
	if(chdir(dir) != -1){
		return;
	}
	strcat(dir,".dir");
	if(chdir(dir) == -1){
		err(1,"chdir %s", dir);
	}
}

int
getDevNull()
{
	int dvn;
	dvn = 0;

	dvn = open("/dev/null",O_RDONLY);
	if(dvn == -1){
		warn("cannot open /dev/null, redirect to standar input");
		statexit = 1;
		return 0;
	}
	return dvn;
}

void
redirectPipe(int fdz, int fdo, int fdread)
{
	int devnull;

	devnull = 0;

	close(fdz);
	if(fdread != -1){
		dup2(fdread,0);
	}else{
		devnull = getDevNull();
		dup2(devnull,0);
	}
	dup2(fdo,1);
	close(fdo);
}

int
pipeTst(char *line, int fdread)
{
	char *cmd;
	char *argv;
	char *path;
	char *pth;
	char *pt;
	int fd[2];

	cmd = strtok(line," ");
	argv = strtok(NULL,"\n");
	path = malloc(PATH_MAX);
	pth = getenv("PATH");
	path = strdup(pth);
	pt = strtok(path,":");

	if(pipe(fd) == -1){
		err(1,"pipe");
	}
	switch(fork()){
	case -1:
		err(1, "fork failed");
		break;
	case 0:
		redirectPipe(fd[0],fd[1],fdread);
		executeCommand(pt,cmd,argv);
	default:
		close(fd[1]);
		return fd[0];
	}
}

int
filterExit(int sts,int fdread)
{
	if(sts != 0){
		return -1;
	}
	return fdread;
}

int
pipeCond(char *line, int fdread)
{
	char *cmd;
	char *argv;
	char *path;
	char *pth;
	char *pt;
	int fd[2];
	int pid, sts;

	cmd = strtok(line," ");
	argv = strtok(NULL,"\n");
	path = malloc(PATH_MAX);
	pth = getenv("PATH");
	path = strdup(pth);
	pt = strtok(path,":");

	if(pipe(fd) == -1){
		err(1,"pipe");
	}
	pid = fork();
	switch(pid){
	case -1:
		err(1, "fork failed");
		break;
	case 0:
		redirectPipe(fd[0],fd[1],fdread);
		executeCommand(pt,cmd,argv);
	default:
		close(fd[1]);
		while(wait(&sts) != pid){
			;
		}
		return filterExit(sts,fd[0]);
	}
}

FILE *
openFile(char *file)
{
	FILE *fd;
	fd = NULL;

	fd = fopen(file,"r");
	if(fd == NULL){
		warn("fopen %s", file);
	}
	return fd;
}

char *
getExt(char *name)
{
	char *tail;

	tail = NULL;

	tail = strstr(name,ext.tst);
	if(tail != NULL){
		return strdup(tail);
	}
	tail = strstr(name,ext.cond);
	if(tail != NULL){
		return strdup(tail);
	}
	tail = strstr(name,ext.ok);
	if(tail != NULL){
		return strdup(tail);
	}
	tail = strstr(name,ext.out);
	if(tail == NULL){
		err(1,"fatal error");
	}
	return strdup(tail);
}

int
execute(char *file)
{
	FILE *fd;
	char buffer[1024*8];
	char *buf;
	int fdread;
	char *ex;

	fd = NULL;
	buf = NULL;
	ex = NULL;
	fdread = -1;

	fd = openFile(file);
	if(fd == NULL){
		return fdread;
	}
	while(fgets(buffer,1024*8,fd) != NULL){
		buf = strdup(buffer);
		if(strcmp(buffer,"\n") == 0){
			continue;
		}
		if(strcmp(strtok(buf," "),"cd") == 0){
			executeChdir(strtok(NULL,"\n"));
			continue;
		}
		buffer[strlen(buffer)-1] = '\0';
		ex = getExt(file);
		if(strcmp(ex,ext.tst) == 0){
			fdread = pipeTst(buffer,fdread);
		}
		if(strcmp(ex,ext.cond) == 0){
			fdread = pipeCond(buffer,fdread);
			if(fdread != -1){
				break;
			}
		}
	}
	fclose(fd);
	while(wait(0) != -1){
		;
	}
	return fdread;
}

int
checkExt(char *name, char *extension)
{
	char *tail;
	int control;
	
	tail = NULL;
	control = 0;
	
	tail = strstr(name,extension);
	if(tail == NULL){
		return -1;
	}
	if(strcmp(tail,extension) != 0){
		control = -1;
	}
	return control;
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
		statexit = 1;
		return -1;
	}
	return 0;
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

DIR *
openDir(char *dir)
{
	DIR *dd;
	dd = NULL;
	
	dd = opendir(dir);
	if(dd == NULL){
		err(1,"opendir");
	}
	return dd;
}

void
saveFile(List *list, char *name)
{

	Cell *cell = malloc(sizeof(Cell));
	(*cell).name = strdup(name);
	(*cell).next = NULL;

	Cell *aux = malloc(sizeof(Cell));

	if ((*list).first == NULL){
		(*list).first = malloc(sizeof(Cell));
		memcpy((*list).first,cell,sizeof(Cell));
		return;
	}
	aux = (*list).first;
	while((*aux).next != NULL){
		aux = (*aux).next;
	}
	(*aux).next = malloc(sizeof(Cell));
	memcpy((*aux).next,cell,sizeof(Cell));
}

int
creatOut(char *file, char *cwd)
{
	char *fout;
	int fd;

	fout = malloc(strlen(file) + strlen(".out")-2);
	fd = 0;

	fout = putoffExt(file,getExt(file));
	strcat(fout,ext.out);
	executeChdir(cwd);
	fd = creat(fout,0660);
	if( fd < 0){
		err(1,"creat");
	}
	return fd;
}

void
fileOut(int fdwrite, int fdread)
{
	int indr;
	int indw;
	char buffer[1024*8];

	indr = 0;
	indw = 0;

	if(fdread == -1){
		return;
	}
	indr = read(fdread,buffer,strlen(buffer));
	while(indr != 0){
		indw = write(fdwrite,buffer,indr);
		if(indr != indw){
			err(1,"write");
		}
		indr = read(fdread,buffer,strlen(buffer));
	}
}

void
tstout(int no)
{
	err(1,"test interrupted");
}

void
processFile(char *cwd, char *file)
{
	int fdr;
	int fdw;

	fdr = 0;
	fdw = 0;

	if(flagTime != 0){
		signal(SIGALRM, tstout);
		siginterrupt(SIGALRM, 1);
		alarm(flagTime);
	}
	fdr = execute(file);
	fdw = creatOut(file,cwd);
	fileOut(fdw,fdr);
	alarm(0);
	exit(EXIT_SUCCESS);
}

void
processDir(char *cwd, List *okL)
{
	DIR *dir;
	char *file;
	char *nf;
	
	dir = NULL;
	file = NULL;
	nf = NULL;

	dir = openDir(cwd);
	for(;;){
		file = readDir(dir);
		if(file == NULL){
			break;
		}
		if(checkRegularFile(file) == -1){
			continue;
		}
		if(checkExt(file,ext.tst) == -1 && checkExt(file,ext.cond) == -1){
			continue;
		};
		nf = putoffExt(file,getExt(file));
		saveFile(okL,nf);
		switch(fork()){
		case -1:
			err(1, "fork failed");
			break;
		case 0:
			processFile(cwd,file);
		default:
			continue;
		}
	}
	while(wait(0) != -1){
		;
	}
}

void
compareOut(FILE *fdok, FILE *fdout, char *fok, char *fout)
{
	char bufok[1024*8];
	char bufout[1024*8];
	char *cok;
	char *cout;

	cok = NULL;
	cout = NULL;

	for(;;){
		cok = fgets(bufok,1024*8,fdok);
		cout = fgets(bufout,1024*8,fdout);
		if((cok == NULL) && (cout != NULL)){
			statexit = 1;
			break;
		}else if((cok != NULL) && (cout == NULL)){
			statexit = 1;
			break;
		}else if((cok == NULL) && (cout == NULL)){
			break;
		}
		if(strcmp(cok,cout) != 0){
			statexit = 1;
			break;
		}
	}
}

int
creatOk(char *fok)
{
	int fd;

	fd = 0;

	fd = creat(fok,0660);
	if( fd < 0){
		err(1,"creat");
	}
	return fd;
}

void
copyFile(char *des,char *origin)
{
	char buff[1024*8];
	int nr;
	int nw;
	int dest;
	int org;

	nr = 0;
	nw = 0;
	dest = 0;
	org = 0;

	dest = open(des,O_WRONLY);
	org = open(origin,O_RDONLY);
	if(dest < 0 || org < 0){
		err(1,"open");
	}
	for(;;){
		nr = read(org, buff, sizeof buff);
		if(nr < 0){
			err(1,"read");
		}
		if(nr == 0){
			break;
		}
		nw = write(dest, buff, nr);
		if (nw != nr){
			err(1, "write");
		}
	}
	close(dest);
	close(org);
}

void
checkOuts(List *okL)
{
	FILE *fdok;
	FILE *fdout;
	char *fok;
	char *fout;
	Cell *aux;

	fdok = NULL;
	fdout = NULL;
	fok = NULL;
	fout = NULL;
	aux = NULL;

	if((*okL).first == NULL){
		return;
	}
	aux = (*okL).first;
	for(;;){
		if(aux == NULL){
			break;
		}
		fok = strdup((*aux).name);
		fout = strdup((*aux).name);
		strcat(fok,ext.ok);
		strcat(fout,ext.out);
		fdok = openFile(fok);
		fdout = openFile(fout);
		if(fdout == NULL){
			aux = (*aux).next;
			continue;
		}
		if(fdok == NULL){
			creatOk(fok);
			copyFile(fok,fout);
			aux = (*aux).next;
			continue;
		}
		compareOut(fdok,fdout,fok,fout);
		aux = (*aux).next;
	}
}

void
deleteFile(char *file)
{
	if(unlink(file) != 0){
		warn("cannot delete %s",file);
		statexit = 1;
	}
}

void
cleanAll(char *cwd)
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
		if(checkRegularFile(file) == -1){
			continue;
		}
		if(checkExt(file,ext.ok) == 0){
			deleteFile(file);
		}
		if(checkExt(file,ext.out) == 0){
			deleteFile(file);
		}
	}
}

void
checkArgs(int argc,char *argv[])
{
	if(argc != 3){
		err(1,"wrong command line, need some time to -t");
	}
	if(strcmp(argv[2],opt.c) == 0){
		err(1,"wrong command line, cannot use -t and -c at the same time");
	}
	flagTime = atoi(argv[2]);
}

char *
getDir()
{
	char *cwd;
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
	char *cd;
	cd = NULL;

	List *okList = malloc(sizeof(List));

	cd = getDir();
	if(argc == 1){
		processDir(cd,okList);
		checkOuts(okList);
		exit(statexit);
	}
	if(strcmp(argv[1],opt.c) == 0){
		cleanAll(cd);
		exit(statexit);
	}
	if(strcmp(argv[1],opt.t) == 0){
		checkArgs(argc,argv);
		processDir(cd,okList);
		checkOuts(okList);
		exit(statexit);
	}
	exit(statexit);
}

// gcc -c -Wall -Wshadow -g cunit.c && gcc -o cunit cunit.o