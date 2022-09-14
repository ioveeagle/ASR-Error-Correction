#include "recog.h"

#define DELIMITER " "	// space
#define MAX_WORD_COUNT	50

typedef struct {
	char *str;
} T;

class CONTEXT2GRAPH{
private:
	UTILITY util;
	int lineNum;
	int terminal;
	int counter;
	int option;
	T *t, *u;
	void getTerminalNode();
	void outputTransition(FILE *f, int s, int e, const char *str, char *outMsg);
	void readIntoRam(char *file);
public:
	CONTEXT2GRAPH();
	~CONTEXT2GRAPH();
	void doContext2graph(char *contextFile, char *graphOut, char *argOption);
};

CONTEXT2GRAPH::CONTEXT2GRAPH(){
	t=NULL;
	lineNum=0;
}

CONTEXT2GRAPH::~CONTEXT2GRAPH(){
	if(t!=NULL){
		for(int i=0;i<lineNum;i++){
			if(t[i].str!=NULL){
				delete [] t[i].str;
				t[i].str=NULL;
			}
		}
		delete [] t;
		t=NULL;
	}
}

void CONTEXT2GRAPH::getTerminalNode(){
	// Calculate the number of tokens in file
	// Return terminal node index
	char msg[MAXLEN], *pch;
	terminal=1; // 0 is startNode
	for(int i=0;i<lineNum;i++){
		strcpy(msg, t[i].str);
		pch=strtok(msg, DELIMITER); // DELIMITER = " ";
		while(pch!=NULL){
			terminal++;
			pch=strtok(NULL, DELIMITER);
		}
	}
	if(option==0) // without silence
		terminal=terminal-lineNum;
}

void CONTEXT2GRAPH::readIntoRam(char *fileName){
	// read context file line by line. Each line contains tokenized string
	char msg[MAXLEN];
	lineNum=util.getFileLineCount(fileName);
	t = new T[lineNum];	
	int len;
	FILE *in=util.openFile(fileName, "rt");
	for(int i=0;i<lineNum;i++){
		fgets(msg, MAXLEN, in);
		len=strlen(msg);
		msg[len-1]='\0';
		t[i].str=new char[len+1];
		strcpy(t[i].str, msg);
	}
	fclose(in);
}

void CONTEXT2GRAPH::outputTransition(FILE *f, int s, int e, const char *str, char *outMsg){
	char term[MAXLEN];
	sprintf(term, "%d\t%d\t%s\n", s, e, str);
	strcpy(outMsg, term);	
	fprintf(f, "%s", outMsg);
	if(e!=terminal)
		counter++;
}

void CONTEXT2GRAPH::doContext2graph(char *contextFile, char *graphOut, char *argOption){
	char msg[MAXLEN], *pch, outMsg[MAXLEN];
	// get option
	option=atoi(argOption);
	if((option<0)||(option>2)){
		printf("Option must be 0~2\n");
		exit(1);
	}
	// read file
	readIntoRam(contextFile);
	getTerminalNode();
	// output
	int wordCount;
	FILE *out=util.openFile(graphOut, "wt");
	counter=0;

	printf("lineNum is %d\nterminal is %d\n", lineNum, terminal);
	for(int i=0;i<lineNum;i++){
		// retrieve each word
		wordCount=0;
		// MAX_WORD_COUNT := 50, actually it's the maximal token number
		u=new T[MAX_WORD_COUNT]; 

		strcpy(msg, t[i].str);
		pch=strtok(msg, DELIMITER);
		u[wordCount].str=new char[strlen(pch)+1];
		strcpy(u[wordCount].str, pch);
		wordCount++;
		while(pch!=NULL){
			pch=strtok(NULL, DELIMITER);
			if(pch!=NULL){
				u[wordCount].str=new char[strlen(pch)+1];
				strcpy(u[wordCount].str, pch);
				wordCount++;
				if(wordCount>=MAX_WORD_COUNT){
					printf("Word count exceeds %d\n", MAX_WORD_COUNT);
					exit(1);
				}
			}
		}
		// output each word token, the first arc is 0->1
		outputTransition(out, 0, counter+1, u[0].str, outMsg);
		if(option==0){ // without sil
			for(int j=1;j<wordCount-1;j++)
				outputTransition(out, counter, counter+1, u[j].str, outMsg);
			if( wordCount > 1 )
				outputTransition(out, counter, terminal, u[wordCount-1].str, outMsg); // link last token to the terminal node
		}
		if(option==1){ // with sil
			for(int j=1;j<wordCount;j++)
				outputTransition(out, counter, counter+1, u[j].str, outMsg);
			if( wordCount >= 1 )
				outputTransition(out, counter, terminal, SIL, outMsg); // insert a SIL tag at the end of utterance
		}
		if(option==2){ // with predifined symbol
			for(int j=1;j<wordCount;j++)
				outputTransition(out, counter, counter+1, u[j].str, outMsg);
			sprintf(msg, "@%d", i);
			outputTransition(out, counter, terminal, msg, outMsg);
		}
		// release memory
		if(u!=NULL){
			delete [] u;
			u=NULL;
		}
	}
	fprintf(out, "%d\n", terminal);
	fclose(out);
}

int main(int argc, char **argv){
	if(argc!=4){
		printf("Usage: %s contextFile graphOut option\n", argv[0]);
		printf("Option\n");
		printf("0: tree without sil\n");
		printf("1: tree with sil\n");
		printf("2: tree with @# for IC apps\n");
		exit(1);
	}

	CONTEXT2GRAPH c2g;
	c2g.doContext2graph(argv[1], argv[2], argv[3]);

	return 0;
}
