#include "recog.h"

class START2TERMINAL{
private:
	float penalty;
	int option;
	int addedTerminal;
	int machineType;
	int *terminal;
	int terminalNum;
	int nodeNum;
	SYMBOLLIST *symbolList;
	int symbolNum;
	NODE_OFFLINE **p2NO;
	LINK *link, **p2k;
	int linkNum;
	int epsIndex;
	void updateLink(LINK **p, int pos, int s, int e, int in, int out, float weight);
	void checkInputStr(char *penaltyStr, char *optionStr);
public:
	START2TERMINAL(GRAPH &graph);
	~START2TERMINAL();
	void doStart2Terminal(char *outFile, char *penalty, char *option);
};

START2TERMINAL::START2TERMINAL(GRAPH &graph){
	p2NO=graph.p2NO;
	machineType=graph.machineType;
	symbolList=graph.symbolList;
	symbolNum=graph.symbolNum;
	nodeNum=graph.nodeNum;

	terminalNum=graph.terminalNum;
	terminal=new int[terminalNum];
	memcpy(terminal, graph.terminal, sizeof(int)*terminalNum);	

	(terminalNum==1)?addedTerminal=terminal[0]:addedTerminal=nodeNum;

	(terminalNum==1)?linkNum=graph.arcNum+1:linkNum=graph.arcNum+terminalNum+1; // 1: s->t or t->s
	link=new LINK[linkNum];
	p2k=new LINK*[linkNum];
	for(int i=0;i<linkNum;i++)
		p2k[i]=link+i;

	UTILITY util;
	epsIndex=util.stringSearch(symbolList, 0, symbolNum-1, EMPTY);
	if(epsIndex==-1){
		printf("%s not exist in symbolList\n", EMPTY);
		exit(1);
	}
}

START2TERMINAL::~START2TERMINAL(){
	if(link!=NULL)
		delete [] link;
	if(p2k!=NULL)
		delete [] p2k;
	if(terminal!=NULL)
		delete [] terminal;
}

void START2TERMINAL::checkInputStr(char *penaltyStr, char *optionStr){
	UTILITY util;
	if(util.isNumeric(penaltyStr)==true)
		penalty=(float)(atof(penaltyStr)/log10(exp(1.0)));
	else{
		printf("Error while converting \"%s\" to numbers\n", penaltyStr);
		exit(1);
	}
	if(util.isNumeric(optionStr)==true){
		option=atoi(optionStr);
		if((option!=0)&&(option!=1)){
			printf("Option is either 0 or 1\n");
			exit(1);
		}
	}
	else{
		printf("Error while converting \"%s\" to numbers\n", optionStr);
		exit(1);
	}
}

void START2TERMINAL::updateLink(LINK **p, int pos, int s, int e, int in, int out, float weight){
	p[pos]->start=s;
	p[pos]->end=e;
	p[pos]->in=in;
	p[pos]->out=out;
	p[pos]->weight=weight;
}

void START2TERMINAL::doStart2Terminal(char *outFile, char *penaltyStr, char *optionStr){
	checkInputStr(penaltyStr, optionStr);

	UTILITY util;
	util.representLink(p2k, p2NO, nodeNum);
	// added transitions from terminal(s) to addedTerminal
	if(terminalNum>1){
		for(int i=0;i<terminalNum;i++)
			updateLink(p2k, linkNum-terminalNum-1+i, terminal[i], addedTerminal, epsIndex, epsIndex, 0);
		if(terminal!=NULL)
			delete [] terminal;
		terminal=new int[1];
		terminal[0]=addedTerminal;
		terminalNum=1;
	}
	// direction
	if(option==0)
		updateLink(p2k, linkNum-terminalNum, 0, addedTerminal, epsIndex, epsIndex, penalty);
	else
		updateLink(p2k, linkNum-terminalNum, addedTerminal, 0, epsIndex, epsIndex, penalty);
	// renumerate and output file
	util.nodeRenumerate(p2k, linkNum, terminal, terminalNum);
	util.outputGraph(p2k, linkNum, terminal, terminalNum, symbolList, machineType, outFile);
}

int main(int argc, char **argv){
	if(argc!=6){
		printf("Usage: %s inGraph outGraph sym penalty option\n", argv[0]);
		printf("option 0 : from start(0) to terminal\n");
		printf("option 1 : from terminal to start(0)\n");
		exit(1);
	}

	GRAPH graph;
	graph.readGraph(argv[1], argv[3], 0);

	START2TERMINAL s2t(graph);
	s2t.doStart2Terminal(argv[2], argv[4], argv[5]);

	return 0;
}
