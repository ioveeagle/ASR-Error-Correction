#include "recog.h"

#define SEGMENTSIZE 100000

class GRAPH2SYMBOL{
private:
	SYMBOLLIST *uSym, *sym;
	int uSymNum, symNum;
	int nodeNum, arcNum, machineType;
	void addSymbol(char *str);
	void combineSymbol();
public:
	GRAPH2SYMBOL();
	~GRAPH2SYMBOL();
	void doGraph2Symbol(char *graphFile, char *symFile);
};

GRAPH2SYMBOL::GRAPH2SYMBOL(){
	uSym=NULL;
	sym=NULL;
}

GRAPH2SYMBOL::~GRAPH2SYMBOL(){
	if(sym!=NULL){
		delete [] sym;
		sym=NULL;
	}
	free(uSym);
}

void GRAPH2SYMBOL::combineSymbol(){
	UTILITY util;
	uSym=(SYMBOLLIST*)realloc(uSym, sizeof(SYMBOLLIST)*(uSymNum+symNum));
	memcpy(uSym+uSymNum, sym, sizeof(SYMBOLLIST)*symNum);
	uSymNum+=symNum;
	util.sortString(uSym, 0, uSymNum-1);
	uSymNum=util.uniqueSymbol(uSym, uSymNum);
}

void GRAPH2SYMBOL::addSymbol(char *str){
	UTILITY util;
	if(str!=NULL){
		if(util.stringSearch(uSym, 0, uSymNum-1, str)==-1){
			strcpy(sym[symNum].str, str);
			symNum++;
			if(symNum==SEGMENTSIZE){
				combineSymbol();				
				symNum=0;
			}
		}
	}
}

void GRAPH2SYMBOL::doGraph2Symbol(char *graphFile, char *symFile){
	UTILITY util;
	typedef struct{
		char str[150];
	}T;

	// obtain symbols
	int fileLineNum, c=0;
	char msg[MAXLEN], *rtn, *p1, *p2, *p3, *p4;

	util.getGraphInfo(graphFile, fileLineNum, nodeNum, arcNum, machineType);
	
	symNum=0;
	uSymNum=0;	// unique symbol number
	uSym=(SYMBOLLIST*)malloc(uSymNum*sizeof(SYMBOLLIST));
	sym = new SYMBOLLIST[SEGMENTSIZE];
	T *t=new T[SEGMENTSIZE];
	FILE *in=util.openFile(graphFile, "rt");
	for(int i=0;i<fileLineNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';	
		p1=strtok(msg, "\t "); // src index
		p2=strtok(NULL, "\t "); // dst index
		p3=strtok(NULL, "\t "); // input label
		p4=strtok(NULL, "\t "); // output label

		addSymbol(p3); // acceptor only needs input labels
		if(machineType==1)
			addSymbol(p4); // transducer needs output labels
	}
	// for a transducer, p3 and p4 may have many identical labels, combineSymbol() will find out unique labels among them.
	if(symNum<SEGMENTSIZE)
		combineSymbol();

	fclose(in);
	// release memory
	if(t!=NULL){
		delete [] t;
		t=NULL;
	}
	// output
	int count=0;
	FILE *out=util.openFile(symFile, "wt");
	fprintf(out, "%s\t%d\n", EMPTY, count);// 0 is reserved for <eps>
	count++;
	for(int i=0;i<uSymNum;i++){
		if(strcmp(uSym[i].str, EMPTY)!=0){
			fprintf(out, "%s\t%d\n", uSym[i].str, count); // 'count' is the symbol index
			count++;
		}
	}
	fclose(out);
}

// extract unique symbols (including input and output labels) from a graph
int main(int argc, char **argv){
	if(argc!=3){
		printf("Usage: %s graphFile symbolFile\n", argv[0]);
		exit(1);
	}
	
	GRAPH2SYMBOL g2s;
	g2s.doGraph2Symbol(argv[1], argv[2]);

	return 0;
}

