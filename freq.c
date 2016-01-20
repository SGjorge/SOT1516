# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
#include <err.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


struct Record{
	char c;
	float count;
	int first;
	int last;
};
typedef struct Record Record;

static  float wordCounter = 0.0;
static char lastChar = 'ç';

int
spacers(char c){
	if(!isalpha(c)){
		return 1;
	}
	if(ispunct(c)){
		return 1;
	}
	return 0;
}

void
initializeRecord(int sizeArray,Record *regs)
{
	int i;
	Record r1;
	r1.c = '0';
	r1.count = 0.0;
	r1.first = 0;
	r1.last = 0;
	
	for(i=0; i<sizeArray; i++){
		regs[i] = r1;
	}
}

void
printRecord(int sizeArray,Record *regs)
{
	int i;
	Record r;
	float percent;
	
	percent = 0.0;
	
	for(i=0; i<sizeArray; i++){
		r = regs[i];
		if(r.c != '0'){;
			printf("%c ",r.c);
			percent = ((r.count/wordCounter)*100.0);
			printf("%.2f ",percent);
			printf("%d ",r.first);
			printf("%d\n",r.last);
		}
	}
}

void
exchangeRecord(Record *regs,int p1, int p2){
	Record rAux;
	char *c1;
	char *c2;
	
	c1 = &regs[p1].c;
	c2 = &regs[p2].c;
	
	if(strcmp(c2,c1) == 1){
		rAux = regs[p1];
		regs[p1] = regs[p2];
		regs[p2] = rAux;
	}
}

void
tidyRecord(int sizeArray,Record *regs)
{
	int j;
	int i;
	
	j = 0;
	i = 0;

	for(i=0; i < sizeArray; i++){
		for(j = 0; j < sizeArray; j++){
			exchangeRecord(regs,i,j);
		}
	}
}

int
lookforReg(Record *regs, char c, int sizeArray)
{
	int i;
	int index;
	index = -1;
	
	for(i =0; i < sizeArray; i++){
		if(regs[i].c == c){
			index = i;
			break;
		}
	}
	return index;
}

void
incrementCount(Record *regs, int index)
{
	regs[index].count ++;
}

void
incrementFirst(Record *regs, int first, int index)
{
	if(first != 0){
		regs[index].first ++;
	}
}

void
incrementLast(Record *regs, int last, int index)
{
	if(last != 0){
		regs[index].last ++;
	}
}

void
safeArgs(Record *regs, char c,int sizeArray, int first, int last)
{
	char chNull;
	int i;
	
	chNull = '0';
	i = 0;
	
	i = lookforReg(regs,c,sizeArray);
	if(i == -1){
		i = lookforReg(regs,chNull,sizeArray);
		regs[i].c = c;
	}
	incrementCount(regs,i);
	incrementFirst(regs,first,i);
	incrementLast(regs,last,i);

}

int
flagFirst(char lastC)
{
	
	switch(lastC){
	case 'ç':
		return 1;
	case '\0':
		return 0;
	}
	if(spacers(lastC)){
		return 1;
	}else{
		return 0;
	}
}

int
flagLast(char *buffer,int index)
{
	if(spacers(buffer[index + 1])){
		return 1;
	}else{
		return 0;
	}
}

char
toLower(int flagLower,char ch)
{
	char c;
	c = '0';
	
	if(flagLower){
		c = tolower(ch);
	}else{
		c = ch;
	}
	return c;
}

void
processBuf(Record *regs, char *buffer, int end,int sizeArray,int flagLower)
{
	int i;
	int first;
	int last;
	char c;
	
	i =0;
	first = 0;
	last = 0;
	c = 'ç';
	
	for(i =0; i < end;i++){
		if(!spacers(buffer[i])){
			c = buffer[i];
			c = toLower(flagLower,c);
			first = flagFirst(lastChar);
			last = flagLast(buffer,i); 
			safeArgs(regs,c,sizeArray,first,last);
			wordCounter ++;
		}else{
			last = flagLast(buffer,i);
		}
		lastChar = buffer[i];
	}
}

int
reader(char *buffer, int fd)
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
process(int fd,Record *regs, char *buffer,int sizeArray,int flagLower)
{
	int end;
	end = 1;
	
	while(end > 0){
		end = reader(buffer,fd);
		processBuf(regs,buffer,end,sizeArray,flagLower);
	}
}

int
openFile(int fd,char *file)
{
	fd = open(file, O_RDONLY);
	if(fd < 0){
		err(1, "%s",file);
		exit(EXIT_FAILURE);
	}
	return fd;
}

int
main(int argc, char *argv[])
{
	int sizeRegs = 54; 
	Record regs[sizeRegs];
	char buffer[8 * 1024];
	int flagLower = 1;
	int fd = 0;
	
	
	initializeRecord(sizeRegs,regs);
	printf("-------------------FREQ-----------------\n");
	if(argc == 1){
		process(fd,regs,buffer,sizeRegs,flagLower);
	}else if(argc == 2){
		switch (strcmp(argv[1],"-i")){
		case 0:
			flagLower = 0;
			process(fd,regs,buffer,sizeRegs,flagLower);
			break;
		default:
			fd = openFile(fd,argv[1]);
			process(fd,regs,buffer,sizeRegs,flagLower);
			close(fd);
		}
	}else if(argc == 3){
		switch (strcmp(argv[1],"-i")){
		case 0:
			flagLower = 0;
			fd = openFile(fd,argv[2]);
			process(fd,regs,buffer,sizeRegs,flagLower);
			close(fd);
			break;
		default:
			err(1,"wrong options");
		}
	}else{
		err(1,"wrong options");
		exit(EXIT_FAILURE);
	}
	tidyRecord(sizeRegs,regs);
	printRecord(sizeRegs,regs);
	
	exit(EXIT_SUCCESS);
}

// gcc -c -Wall -Wshadow -g freq.c && gcc -o freq freq.o