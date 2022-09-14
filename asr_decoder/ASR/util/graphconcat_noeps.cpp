#include "recog.h"

#define MAX_TERMINAL_NUM	1000000

class CONCATE{
private:
	int *srcTerminal;
	int srcTerminalNum;
	int machineType;
	int accumulateNodeNum;
	void outputString(FILE *out, char *inFile, int accumOption);
	int obtainMaxNode(char *p1, char *p2, int maxNode);
	void checkGraphType(int argc, char **argv);
public:
	CONCATE();
	~CONCATE();	
	void doConcatenation(int argc, char **argv);
};

CONCATE::CONCATE(){
	srcTerminal=NULL;
}

CONCATE::~CONCATE(){
	if(srcTerminal!=NULL)
		delete [] srcTerminal;
}

int CONCATE::obtainMaxNode(char *p1, char *p2, int maxNode){
	int s, e;

	s=atoi(p1);
	(p2!=NULL)?e=atoi(p2):e=-1;

	if(s>maxNode)
		maxNode=s;
	if(e>maxNode)
		maxNode=e;

	return maxNode;
}

void CONCATE::outputString(FILE *out, char *inFile, int accumOption){
	// output all transitions except terminal(s)
	int maxNode=-1;
	char msg[MAXLEN], *p1, *p2, *p3, *rtn;

	UTILITY util;
	int lineNum=util.getFileLineCount(inFile);
	FILE *in=util.openFile(inFile, "rt");
	srcTerminalNum=0;
	for(int i=0;i<lineNum;i++){
		rtn=fgets(msg, MAXLEN, in);
		msg[strlen(msg)-1]='\0';
		p1=strtok(msg, "\t "); // src index
		p2=strtok(NULL, "\t "); // dst index
		p3=strtok(NULL, "");//remaining string
		if(p2!=NULL)
			fprintf(out, "%d\t%d\t%s\n", atoi(p1)+accumulateNodeNum, atoi(p2)+accumulateNodeNum, p3);
		else{ // manipulate terminal node, which only has src index
			srcTerminal[srcTerminalNum]=atoi(p1)+accumulateNodeNum; // record the index of terminal node
			srcTerminalNum++;
			if(srcTerminalNum>=MAX_TERMINAL_NUM){
				printf("TerminalNum exceeds %d\n", MAX_TERMINAL_NUM);
				exit(1);
			}
		}
		// just find the maximal number of states in the current graph
		maxNode=obtainMaxNode(p1, p2, maxNode);
	}
	fclose(in);

	if(accumOption==1)
		accumulateNodeNum+=(maxNode+1);
}

void CONCATE::doConcatenation(int argc, char **argv){
	checkGraphType(argc, argv);

	UTILITY util;
	char srcFileName[MAXLEN], dstFileName[MAXLEN];
	strcpy(dstFileName, argv[argc-1]);

	srcTerminal = new int[MAX_TERMINAL_NUM];
	srcTerminalNum=0;
	accumulateNodeNum=0;

	FILE *out=util.openFile(dstFileName, "wt");
	outputString(out, argv[1], 1); // manipulate src 1
	for(int i=2;i<argc-1;i++){ // manipulate src 2-src N
		strcpy(srcFileName, argv[i]);
		// from src(i) to src(i+1) by <eps>
		for(int p=0;p<srcTerminalNum;p++){
			fprintf(out, "%d\t%d\t%s", srcTerminal[p], accumulateNodeNum, EMPTY); // EMPTY := <eps>
			(machineType==0)?fprintf(out, "\n"):fprintf(out, "\t%s\n", EMPTY);
		}		
		(i<argc-2)?outputString(out, srcFileName, 1):outputString(out, srcFileName, 0);		
	}
	// output terminal(s)
	for(int j=0;j<srcTerminalNum;j++)
		fprintf(out, "%d\n", srcTerminal[j]);

	fclose(out);
}

void CONCATE::checkGraphType(int argc, char **argv){
	UTILITY util;
	int lineNum, tmpType, nodeNum, arcNum;
    
	// machine types of all graphs should be the same.
	util.getGraphInfo(argv[1], lineNum, nodeNum, arcNum, machineType);
	for(int i=2;i<argc-1;i++){
		util.getGraphInfo(argv[i], lineNum, nodeNum, arcNum, tmpType);
		if(tmpType!=machineType){
			printf("Graph type not consistent, check %s\n", argv[i]);
			exit(1);
		}
	}
}

int main(int argc, char **argv){
	if(argc<=3){
		printf("Usage: %s src1 src2 ... dst\n", argv[0]);
		printf("It concatenates graph src[1~n] to dst.\n");
		exit(1);
	}

	CONCATE concate;	
	concate.doConcatenation(argc, argv);

	return 0;
}
