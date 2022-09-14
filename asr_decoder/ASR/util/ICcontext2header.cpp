#include "recog.h"

#define DELIMITER " "	// space

typedef struct {
	char *str;
	int wordNum;
} T;

class CONTEXT2GRAPH{
private:
	int lineNum;
	int terminal;
	int counter;
	int contextWordNum;
	int maxWordLength;
	T *t;
	SYMBOLLIST *symbolList;
	int symbolNum;
	int *encodedContextWord;
	void encodeContext();
	void obtainSymbol();
	void readIntoMemory(char *contextFile);
	void outputTransition(FILE *f, int s, int e, const char *str, char *outMsg);
	void outputHeader(char *contextHeader);
	void getTerminalNode(int option);
public:
	CONTEXT2GRAPH();
	~CONTEXT2GRAPH();
	void doContext2graph(char *contextFile, char *contextHeader);
};

CONTEXT2GRAPH::CONTEXT2GRAPH(){
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
	if(encodedContextWord!=NULL){
		delete [] encodedContextWord;
		encodedContextWord=NULL;
	}
	if(symbolList!=NULL){
		delete [] symbolList;
		symbolList=NULL;
	}
}

void CONTEXT2GRAPH::outputTransition(FILE *f, int s, int e, const char *str, char *outMsg){
	char term[MAXLEN];
	sprintf(term, "%d\t%d\t%s\n", s, e, str);
	strcpy(outMsg, term);	
	fprintf(f, "%s", outMsg);
	if(e!=terminal)
		counter++;
}

void CONTEXT2GRAPH::obtainSymbol(){
	UTILITY util;
	// word num
	contextWordNum=0;
	for(int i=0;i<lineNum;i++)
		contextWordNum+=t[i].wordNum;
	// get unique symbol
	symbolList=new SYMBOLLIST[contextWordNum+1]; // add <eps>	
	int cnt=0, pchLen;
	char msg[MAXLEN], *pch;
	strcpy(symbolList[cnt].str, EMPTY);
	cnt++;
	maxWordLength=strlen(EMPTY);
	for(int i=0;i<lineNum;i++){
		strcpy(msg, t[i].str);
		pch=strtok(msg, DELIMITER);
		while(pch!=NULL){
			strcpy(symbolList[cnt].str, pch);
			cnt++;
			pchLen=strlen(pch);
			if(pchLen>maxWordLength)
				maxWordLength=pchLen;
			pch=strtok(NULL, DELIMITER);
		}
	}
	symbolNum=util.uniqueSymbol(symbolList, cnt);
}

void CONTEXT2GRAPH::encodeContext(){// context encoded by sym
	UTILITY util;

	int w, cnt=0;
	char msg[MAXLEN], *pch;

	encodedContextWord= new int[contextWordNum];

	for(int i=0;i<lineNum;i++){
		strcpy(msg, t[i].str);
		pch=strtok(msg, DELIMITER);
		w=util.stringSearch(symbolList, 0, symbolNum-1, pch);
		if(w==-1){
			printf("%s not found in syms\n", pch);
			exit(1);
		}
		encodedContextWord[cnt]=w;
		cnt++;
		for(int j=1;j<t[i].wordNum;j++){
			pch=strtok(NULL, DELIMITER);
			w=util.stringSearch(symbolList, 0, symbolNum-1, pch);
			if(w==-1){
				printf("%s not found in syms\n", pch);
				exit(1);
			}
			encodedContextWord[cnt]=w;
			cnt++;
		}
	}
}

void CONTEXT2GRAPH::outputHeader(char *contextHeader){
	UTILITY util;

	int *pos=new int[lineNum];
	int *len=new int[lineNum];

	// length
	for(int i=0;i<lineNum;i++)
		len[i]=t[i].wordNum;
	// access pos
	pos[0]=0;
	for(int i=1;i<lineNum;i++)
		pos[i]=pos[i-1]+len[i-1];

	// output
	char msg[MAXLEN], tmp[MAXLEN];
	FILE *out=util.openFile(contextHeader, "wt");
	fprintf(out, "// ---------- CONTEXT ----------\n");
	// symbol
	fprintf(out, "const short int symbolNum=%d;\n", symbolNum);
	int epsIndex=util.stringSearch(symbolList, 0, symbolNum-1, EMPTY);
	fprintf(out, "const short int epsIndex=%d;\n", epsIndex);
	fprintf(out, "const char symbolList[%d][%d]={", symbolNum, maxWordLength+1);
	strcpy(msg, "");
	for(int i=0;i<symbolNum;i++){
		sprintf(tmp, "\"%s\",", symbolList[i].str);
		strcat(msg, tmp);
		if(i+1<symbolNum){
			if((i!=0)&&(i%15==0)){
				fprintf(out, "%s\n", msg);
				strcpy(msg, "");
			}
		}
		else{
			msg[strlen(msg)-1]='\0';
			fprintf(out, "%s};\n\n", msg);
		}
	}
	// context info
	fprintf(out, "const short int sentenceNum=%d;\n", lineNum);
	fprintf(out, "const short int sentencePos[%d]={", lineNum);
	util.writeVector(out, pos, lineNum, 30);
	fprintf(out, "const short int sentenceLength[%d]={", lineNum);
	util.writeVector(out, len, lineNum, 30);
	fprintf(out, "const short int contextWord[%d]={", contextWordNum);
	util.writeVector(out, encodedContextWord, contextWordNum, 30);
	fclose(out);

	delete [] pos;
	delete [] len;
}

void CONTEXT2GRAPH::readIntoMemory(char *contextFile){
	UTILITY util;

	lineNum=util.getFileLineCount(contextFile);
	t = new T[lineNum];
	// read into memory
	int len, cnt;
	char msg[MAXLEN], *pch;
	FILE *in=util.openFile(contextFile, "rt");
	for(int i=0;i<lineNum;i++){
		// string
		fgets(msg, MAXLEN, in);
		len=strlen(msg);
		msg[len-1]='\0';
		t[i].str=new char[len+1];
		strcpy(t[i].str, msg);
		// word number
		cnt=0;
		pch=strtok(msg, DELIMITER);
		while(pch!=NULL){
			cnt++;
			pch=strtok(NULL, DELIMITER);
		}
		t[i].wordNum=cnt;
	}
	fclose(in);
}

void CONTEXT2GRAPH::getTerminalNode(int option){
	char msg[MAXLEN], *pch;
	terminal=1; // 0 is startNode
	for(int i=0;i<lineNum;i++){
		strcpy(msg, t[i].str);
		pch=strtok(msg, DELIMITER);
		while(pch!=NULL){
			terminal++;
			pch=strtok(NULL, DELIMITER);
		}
		terminal--;
	}
	// option indicates including sil or not
	(option==0)?terminal=terminal:terminal+=lineNum;
}

void CONTEXT2GRAPH::doContext2graph(char *contextFile, char *contextHeader){
	UTILITY util;

	readIntoMemory(contextFile);
	getTerminalNode(1);
	obtainSymbol();
	encodeContext();

	outputHeader(contextHeader);
}

int main(int argc, char **argv){
	if(argc!=3){
		printf("Usage: %s contextFile context.h\n", argv[0]);
		exit(1);
	}

	CONTEXT2GRAPH c2g;
	c2g.doContext2graph(argv[1], argv[2]);

	return 0;
}
