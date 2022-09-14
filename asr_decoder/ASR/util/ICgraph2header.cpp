#include "recog.h"

class ICGRAPH2HEADER {
private:
	GRAPH G;
	int epsIndex;
	void write2file(char *graphFile, char *headerFile);
public:
	ICGRAPH2HEADER();
	~ICGRAPH2HEADER();
	void doGraph2Header(char *graphFile, char *macroFile, char *symFile, char *headerFile);
};

ICGRAPH2HEADER::ICGRAPH2HEADER(){}

ICGRAPH2HEADER::~ICGRAPH2HEADER(){}

void ICGRAPH2HEADER::write2file(char *graphFile, char *headerFile){
	UTILITY util;
	// output
	int *vec;
	FILE *out=util.openFile(headerFile, "wt");
	fprintf(out, "//---------- GRAPH ----------\n");
	fprintf(out, "const short int nodeNum=%d;\n", G.nodeNum);
	fprintf(out, "const short int arcNum=%d;\n", G.arcNum);
	fprintf(out, "const short int terminalNum=%d;\n\n", G.terminalNum);
	// terminal
	fprintf(out, "const short int terminal[%d]={", G.terminalNum);
	vec=new int[G.terminalNum];
	for(int i=0;i<G.terminalNum;i++)
		vec[i]=G.terminal[i];
	util.writeVector(out, vec, G.terminalNum, 20);
	delete [] vec;
	// node branchNum
	fprintf(out, "const short int nodeBranchNum[%d]={", G.nodeNum);
	vec=new int[G.nodeNum];
	for(int i=0;i<G.nodeNum;i++)
		vec[i]=G.p2N[i]->branchNum;
	util.writeVector(out, vec, G.nodeNum, 40);
	delete [] vec;
	// arc2Node
	int cnt=0;
	fprintf(out, "const short int arc2Node[%d]={", G.arcNum);
	vec=new int[G.arcNum];
	for(int i=0;i<G.nodeNum;i++){
		for(int j=0;j<G.p2N[i]->branchNum;j++){
			vec[cnt]=i;
			cnt++;
		}
	}
	util.writeVector(out, vec, G.arcNum, 40);
	delete [] vec;
	// node2arc
	fprintf(out, "const short int node2arc[%d]={", G.nodeNum);
	vec=new int[G.nodeNum];
	for(int i=0;i<G.nodeNum;i++)
		vec[i]=G.p2N[i]->arcPos;
	util.writeVector(out, vec, G.nodeNum, 40);
	delete [] vec;
	// arc.in
	fprintf(out, "const short int arcIn[%d]={", G.arcNum);
	vec=new int[G.arcNum];
	for(int i=0;i<G.arcNum;i++)
		vec[i]=G.arc[i].in;
	util.writeVector(out, vec, G.arcNum, 30);
	delete [] vec;
	// arc.out, convert out-symbol @# to # (only keep @# ones)
	char msg[MAXLEN], *pch;
	fprintf(out, "const short int arcOut[%d]={", G.arcNum);
	vec=new int[G.arcNum];	
	for(int i=0;i<G.arcNum;i++){
		vec[i]=-1;
		strcpy(msg, G.symbolList[G.arc[i].out].str);
		if(msg[0]=='@'){
			pch=strtok(msg, "@");
			vec[i]=atoi(pch);
		}
	}
	util.writeVector(out, vec, G.arcNum, 30);
	delete [] vec;
	// arc.endNode
	fprintf(out, "const short int arcEndNode[%d]={", G.arcNum);
	vec=new int[G.arcNum];
	for(int i=0;i<G.arcNum;i++)
		vec[i]=G.arc[i].endNode;
	util.writeVector(out, vec, G.arcNum, 30);
	delete [] vec;	

	fclose(out);
}

void ICGRAPH2HEADER::doGraph2Header(char *graphFile, char *macroFile, char *symFile, char *headerFile){
	UTILITY util;
	// read macro
	MACRO macro;
	macro.readMacroBin(macroFile);
	// read graph
	G.readGraph2Decode(graphFile, symFile, 1, 1);
	// output
	write2file(graphFile, headerFile);
}

int main(int argc, char **argv){
	if(argc!=5){
		printf("Usage: %s DEST.graph DEST.macro DEST.sym graph.h\n", argv[0]);
		printf("assign outLabel index\n");
		exit(1);
	}

	ICGRAPH2HEADER ICg2h;
	ICg2h.doGraph2Header(argv[1], argv[2], argv[3], argv[4]);

	return 0;
};
