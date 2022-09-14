#include "recog.h"

#define GRAPHFILE "DEST.graph"
#define DICFILE "DEST.dic"
#define SYMBOLFILE "DEST.sym"

class SOURCE2DEST {
private:
	ARC *arc;
	DIC *dic;
	NODE_OFFLINE **p2NO;
	SYMBOLLIST *symbolList, *slt;
	int symbolNum;
	int nodeNum;
	int arcNum;
	MASTERMACROFILE **p2MMF;
	int modelNum;
	int *terminal;
	int terminalNum;
	void inLabel2MMFIndex();
public:
	SOURCE2DEST(GRAPH &graph, MACRO &macro);
	~SOURCE2DEST();
	void write2DicBin(char *srcfileName);
	void write2GraphBin();
	void write2SymbolBin();
};

SOURCE2DEST::SOURCE2DEST(GRAPH &graph, MACRO &macro){
	p2NO=graph.p2NO;
	nodeNum=graph.nodeNum;
	arcNum=graph.arcNum;
	symbolList=graph.symbolList;
	symbolNum=graph.symbolNum;
	p2MMF=macro.p2MMF;
	terminal=graph.terminal;
	terminalNum=graph.terminalNum;

	modelNum=macro.modelNum;

	slt=new SYMBOLLIST[modelNum];

	for(int i=0;i<modelNum;i++)
		strcpy(slt[i].str, p2MMF[i]->str);
	
	arc=new ARC[arcNum];
}

SOURCE2DEST::~SOURCE2DEST(){
	if(arc!=NULL){
		delete [] arc;
		arc=NULL;
	}
	if(dic!=NULL){
		delete [] dic;
		dic=NULL;
	}
	if(slt!=NULL){
		delete [] slt;
		slt=NULL;
	}
}

void SOURCE2DEST::inLabel2MMFIndex(){
	UTILITY util;
	int idx;
	bool symbolNotFound = false;
	char str[WORDLEN];

	for(int i=0;i<arcNum;i++){
		// NOTE: 'arc[i].in' is negative because of D=# cases
		if( arc[i].in > -1 ){			
			strcpy(str, symbolList[arc[i].in].str);
			if(strcmp(str, EMPTY)!=0){
				idx=util.stringSearch(slt, 0, modelNum-1, str);
				if(idx==-1){
					printf("%s not found in Macro@arc %d (ID:%d) %s\n", str, i, arc[i].in, 
						symbolList[arc[i].in].str);
					symbolNotFound = true;
					//exit(1);
				}
				arc[i].in=idx;
			}
			else
				arc[i].in=-1;
		}
	}
	
	if( symbolNotFound ){
		printf("SOURCE2DEST::inLabel2MMFIndex() exit\n");
		exit(0);
	}
}

void SOURCE2DEST::write2GraphBin(){
	int c=0;
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<p2NO[i]->branchNum;j++){
			arc[c].in=p2NO[i]->arc[j].in;
			arc[c].out=p2NO[i]->arc[j].out;
			arc[c].weight=p2NO[i]->arc[j].weight;
			arc[c].endNode=p2NO[i]->arc[j].endNode;
			//if(c==9196) printf("%d -> %d i:%d o:%d", i, arc[c].endNode, arc[c].in, arc[c].out);
			c++;
			
		}
	}
	inLabel2MMFIndex();

	unsigned short int *branchNumArray = new unsigned short int[nodeNum];
	for(int i=0;i<nodeNum;i++)
		branchNumArray[i]=p2NO[i]->branchNum;

	// write to a binary file
	UTILITY util;
	FILE *out=util.openFile(GRAPHFILE, "wb");
	fwrite(&nodeNum, sizeof(int), 1, out);
	fwrite(&arcNum, sizeof(int), 1, out);
	fwrite(&terminalNum, sizeof(int), 1, out);
	fwrite(branchNumArray, sizeof(unsigned short int), nodeNum, out);
	fwrite(arc, sizeof(ARC), arcNum, out);
	fwrite(terminal, sizeof(int), terminalNum, out);
	fclose(out);

	delete [] branchNumArray;
}

void SOURCE2DEST::write2DicBin(char *srcFileName){
	UTILITY util;
	int dicEntryNum=util.getFileLineCount(srcFileName), foundPos;
	dic = new DIC[dicEntryNum];
	for(int i=0;i<dicEntryNum;i++){
		dic[i].word=-1;
		memset(dic[i].phone, -1, sizeof(int)*MAX_PHONE_NUM);
	}

	FILE *in=util.openFile(srcFileName, "rt");
	char msg[MAXLEN], *pch, *rtn;
	for(int i=0;i<dicEntryNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		
		dic[i].phoneNum=0;
		pch=strtok(msg, "\t ");
		while(pch!=NULL){
			foundPos=util.stringSearch(slt, 0, modelNum-1, pch);
			if(foundPos!=-1){
				dic[i].phone[dic[i].phoneNum]=foundPos;
				dic[i].phoneNum++;
				if(dic[i].phoneNum==MAX_PHONE_NUM){
					printf("Dic entry %d, phoneNum exceeds MAX_PHONE_NUM %d\n", i+1, MAX_PHONE_NUM);
					exit(1);
				}
			}
			else{
				printf("Error finding %s in Acoustic models.\n", pch);
				exit(1);
			}

			pch=strtok(NULL, "\t ");
		}
	}
	fclose(in);
	
	FILE *out=util.openFile(DICFILE, "wb");
	fwrite(&dicEntryNum, 1, sizeof(int), out);
	fwrite(dic, dicEntryNum, sizeof(DIC), out);
	fclose(out);
}

void SOURCE2DEST::write2SymbolBin(){
	UTILITY util;
	FILE *out=util.openFile(SYMBOLFILE, "wb");
	fwrite(&symbolNum, sizeof(int), 1, out);
	fwrite(symbolList, sizeof(SYMBOLLIST), symbolNum, out);
	fclose(out);
}

int main(int argc, char **argv){
	if(argc!=5){
		printf("Usage: %s graph sym dic macro.float.bin\n", argv[0]);
		printf("Convert graph/sym/dic from ascii to binary\n");
		printf("dic comes from the output of partialexpand\n");
		exit(1);
	}	
	char graphFile[MAXLEN], macroFile[MAXLEN], symFile[MAXLEN], dicFile[MAXLEN];
	strcpy(graphFile, argv[1]);
	strcpy(symFile, argv[2]);
	strcpy(dicFile, argv[3]);
	strcpy(macroFile, argv[4]);	

	MACRO macro;
	macro.readMacroBin(macroFile);

	GRAPH graph;
	graph.readGraph(graphFile, symFile, 1);

	SOURCE2DEST s2d(graph, macro);
	s2d.write2GraphBin();
	s2d.write2DicBin(dicFile);
	s2d.write2SymbolBin();

	return 0;
}
