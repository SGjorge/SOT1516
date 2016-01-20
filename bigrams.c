#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/mman.h>
#include <pthread.h>

#define _GNU_SOURCE

enum{
	minArgs = 3,
	maxArgs = 20,
	asciiRange = 128,
	maxSat = 255,
	posMap = 1,
};

struct DataTh{
	char *file;
	char *map;
	pthread_mutex_t lock;
};
typedef struct DataTh DataTh;

struct Projection{
	unsigned char *proj;
	pthread_mutex_t lock;
};
typedef struct Projection Projection;

void*
getPixmap(char *path)
{
	int fd;
	void *addr;

	fd = 0;

	fd = open(path, O_RDWR);
	if(fd < 0) {
		err(1, "open");
	}
	addr = mmap(NULL, asciiRange*asciiRange, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED) {
		err(1, "mmap");
	}
 	close(fd);
 	return addr;
}

void
printPixmap(char *map)
{
	int i;
	int j;
	unsigned char *buffer;

	buffer = getPixmap(map);

	for(i=0; i<asciiRange; i++){
		for(j=0; j<asciiRange; j++){
			fprintf(stdout, "(%d,%d): %d\n",i,j,buffer[(i*asciiRange)+j]);
		}
	}
}

void
writePixmap(char *buffer, Projection *p)
{
	int raw;
	int column;
	int i;
	int value;

	raw = 0;
	column = 0;
	value = 0;

	pthread_mutex_lock(&p->lock);
	for(i=0; i<strlen(buffer)-1; i++){
		raw = buffer[i];
		column = buffer[i+1];
		value = (*p).proj[(raw*asciiRange)+column];
		if(value >= maxSat){
			break;
		}
		(*p).proj[(raw*asciiRange)+column] = value + 1;
	}
	pthread_mutex_unlock(&p->lock);
}

void
erreaseBuffer(char *buf)
{
	int i;

	i = 0;

	for(i=strlen(buf); i >= 0; i--){
		buf[i] = '\0';
	}
}

void
processFile(char *file, char *map)
{
	FILE *fd;
	char buffer[1024*8];
	char buf[1024*8];
	int first;
	Projection *p;

	fd = NULL;
	p = malloc(sizeof(Projection));
	first = 0;

	fd = fopen(file,"r");
	
	if(fd == NULL){
		err(1,"fopen");
	}
	(*p).proj = getPixmap(map);
	while(fgets(buffer,sizeof(buffer),fd) != 0){
		if(first == 0){
			first = 1;
			writePixmap(buffer,p);
			buf[0] = buffer[strlen(buffer)-1];
			strcpy(buf,buf);
		}else{
			strcat(buf,buffer);
			writePixmap(buf,p);
			erreaseBuffer(buf);
			buf[0] = '\n';
		}
	}
	fclose(fd);
}

void *
tPF(void *data)
{
	DataTh *dt;
	char *file;

	dt = data;

	file = malloc(sizeof((*dt).file));
	strcpy(file,(*dt).file);
	pthread_mutex_unlock(&dt->lock);

	processFile((*dt).file,(*dt).map);
	free(file);
	return 0;
}

void
createPixmap(char *path,int size)
{
	int fd;

	fd = 0;

	fd = creat(path,0660);
	if(fd < 0){
		err(1,"creat");
	}
	lseek(fd, size-1, 0);
	if(write(fd, "\0", 1) != 1) {
		err(1, "write");
	}
	close(fd);
}

void
checkArgs(int argc, char *argv[])
{
	if(argc < minArgs){
		fprintf(stderr, "Wrong Command Line\n");
		exit(EXIT_FAILURE);
	}else if(argc > maxArgs){
		fprintf(stderr, "Wrong Command Line, Too Long\n");
		exit(EXIT_FAILURE);	
	}
}

int
main(int argc, char *argv[])
{
	int i;
	DataTh *dt;
	pthread_t thr[maxArgs];
	void *sts[maxArgs];
	int posPix;

	posPix = posMap;
	i = 0;
	dt = malloc(sizeof(DataTh));

	checkArgs(argc,argv);
	if(strcmp(argv[1],"-p") == 0){
		printPixmap(argv[posPix+1]);
		exit(EXIT_SUCCESS);
	}

	(*dt).map = malloc(strlen(argv[posPix]));
	strcpy((*dt).map,argv[posPix]);
	fprintf(stdout, "-------------------- bigrams ---------------------\n");
	createPixmap(argv[posPix],asciiRange*asciiRange);
	for(i=posPix+1; i < argc; i++){
		pthread_mutex_lock(&dt->lock);
		(*dt).file = malloc(strlen(argv[i]));
		strcpy((*dt).file,argv[i]);
		if(pthread_create(thr+i, NULL, tPF, dt) != 0) {
			err(1, "thread");
		}
	}
	for(i = posPix+1; i < argc; i++) {
		pthread_join(thr[i], sts+i);
		free(sts[i]);
	}

	exit(EXIT_SUCCESS);
}