/* Jorge Simón Gil */
/* Tecnologías */
/* tok.c */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int
spacers(char c)
{
	if(c == ' '){
		return 1;
	}else if(c == '\n'){
		return 1;
	}else if(c == '\t'){
		return 1;
	}else if(c == '\r'){
		return 1;
	}else{
		return 0;
	}
}

int
lookFor(char *str,int lenstr)
{
	int i;
	i = 0;
	
	for(i=0; !spacers(str[i]); i++){
		if(i == lenstr){
			break;
		}
	}
	
	return i+1;
}

void
changeToEnd(char *str, int tok)
{
	str[tok-1] = '\0';
}

int
saveTok(char *str, char **args, int tok, int total)
{
	if(!spacers(str[tok])){
		args[total] = &str[tok];
		return total + 1;
	}
	return total;
}

int
mytokenize(char *str, char **args, int maxargs)
{

	int lenstr;
	lenstr = strlen(str);
	int tok;
	tok = 0;
	int total;
	total = 0;
	
	while(total < maxargs && str[tok] != '\0'){
		total = saveTok(str,args,tok,total);
		if(total == maxargs){
			break;
		}
		tok = lookFor(str,lenstr);
		changeToEnd(str,tok);
	}
	
	return total;
}

void
printTokens(char **args, int numsToks)
{
	int i;
	
	for(i=0; i<numsToks; i++){
		printf("word: %s\n",args[i]);
	}
}

int
main(int argc, char *argv[])
{
	int maxargs;
	maxargs = 4;
	int numsToks;
	numsToks = 0;
	char *args[maxargs];
	char str1[] = "cadena de  texto ";
	
	numsToks = mytokenize(str1,args,maxargs);
	printTokens(args,numsToks);
	printf("total tokens: %d\n",numsToks);
	
	exit(EXIT_SUCCESS);
}