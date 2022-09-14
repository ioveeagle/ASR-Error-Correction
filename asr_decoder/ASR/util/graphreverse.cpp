#include "recog.h"

class GRAPHREVERSE{
private:
	NODE_OFFLINE **p2NO;
	int nodeNum;
	SYMBOLLIST *symbolList;
	int symbolNum;
	LINK *link, **p2k;
	int linkNum;
	int *terminal;
	int terminalNum;
	int machineType;
	int epsIndex;
	void determineEntryPoint();
public:
	GRAPHREVERSE(GRAPH &graph);
	~GRAPHREVERSE();
	void doReverse(char *outFile);
};

GRAPHREVERSE::GRAPHREVERSE(GRAPH &graph){
	p2NO=graph.p2NO;
	nodeNum=graph.nodeNum;
	symbolList=graph.symbolList;
	symbolNum=graph.symbolNum;
	machineType=graph.machineType;
	// if original graph has 'terminalNum' terminal nodes, there will be 'terminalNum' links added to the reversed graph
	(graph.terminalNum>1)?linkNum=graph.arcNum+graph.terminalNum:linkNum=graph.arcNum;
	link=new LINK[linkNum];
	p2k=new LINK*[linkNum];
	for(int i=0;i<linkNum;i++)
		p2k[i]=link+i;
	//terminal
	terminal=new int[graph.terminalNum];
	terminalNum=graph.terminalNum;
	memcpy(terminal, graph.terminal, sizeof(int)*terminalNum);

	UTILITY util;
	epsIndex=util.stringSearch(symbolList, 0, symbolNum, EMPTY);
	if(epsIndex==-1){
		printf("%s does not exist in symbolList\n", EMPTY);
		exit(1);
	}
}

GRAPHREVERSE::~GRAPHREVERSE(){
	if(link!=NULL)
		delete [] link;
	if(p2k!=NULL)
		delete [] p2k;
	if(terminal!=NULL)
		delete [] terminal;
}

void GRAPHREVERSE::determineEntryPoint(){
	// so far there is only one terminal
	for(int i=0;i<linkNum;i++){
		if(p2k[i]->start==terminal[0])
			p2k[i]->start=0;
		if(p2k[i]->end==0)
			p2k[i]->end=terminal[0];
	}
}

void GRAPHREVERSE::doReverse(char *outFile){
	int newNode=nodeNum;
	UTILITY util;
	util.representLink(p2k, p2NO, nodeNum);
	if(terminalNum>1){
		// in reversed graph, original terminal nodes are linking to a new entry node.
		// these links are labeled <eps>:<eps>/0
		for(int i=0;i<terminalNum;i++){
			p2k[linkNum-terminalNum+i]->start=terminal[i];
			p2k[linkNum-terminalNum+i]->end=newNode;
			p2k[linkNum-terminalNum+i]->in=epsIndex;
			p2k[linkNum-terminalNum+i]->out=epsIndex;
			p2k[linkNum-terminalNum+i]->weight=0;
		}
		// update terminal
		if(terminal!=NULL)
			delete [] terminal;
		terminalNum=1;
		terminal=new int[1];
		terminal[0]=newNode;
	}
	// S E -> E S
	int tmp;
	for(int i=0;i<linkNum;i++){
		tmp=p2k[i]->start;
		p2k[i]->start=p2k[i]->end;
		p2k[i]->end=tmp;
	}
	// determine entry point
	determineEntryPoint();

	util.nodeRenumerate(p2k, linkNum, terminal, terminalNum);
	util.outputGraph(p2k, linkNum, terminal, terminalNum, symbolList, machineType, outFile);
}

int main(int argc, char **argv){
	if(argc!=4){
		printf("Usage: %s graph.in graph.out sym\n", argv[0]);
		exit(1);
	}
	char inFile[MAXLEN], outFile[MAXLEN], symbolListFile[MAXLEN];
	strcpy(inFile, argv[1]);
	strcpy(outFile, argv[2]);
	strcpy(symbolListFile, argv[3]);

	GRAPH graph;
	graph.readGraph(inFile, symbolListFile, 0);

	GRAPHREVERSE gr(graph);
	gr.doReverse(outFile);

	return 0;
}
