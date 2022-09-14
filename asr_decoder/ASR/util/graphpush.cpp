#include "recog.h"

#define traverseDepth	50
#define DEBUG_MODE 0
typedef struct {
	int dstNode;
	int dstBranch;
	int arcNum;
	int endNode;
	int arc[traverseDepth];
} TOPHONE;

class GRAPHPUSH {
private:
	NODE_OFFLINE **p2NO;
	SYMBOLLIST *symbolList;
	int symbolNum;
	int epsIndex, silIndex, fstrIndex;
	int nodeNum;
	int machineType;
	int *terminal;
	int terminalNum;
	TOPHONE *tp;
	int traverseNum;
	int pathNum;
	int *inDegree;
	int *doMove;
	void getInDegree();
	bool nodeProceed(int node);
	void str2index(SYMBOLLIST *sl, int slNum, const char *str, int &strIndex);
	void showPath(int node);
	void addTransition(int node, int branch);
	void toWordBoundary(int node);
	void initialize();
	bool reachWordBoundary(int node, int branch);
	void moveOutlabelandWeight(int node, int branch);
	void outputGraph(char *outGraph);
public:
	GRAPHPUSH(GRAPH &graph);
	~GRAPHPUSH();
	void pushForward(char *outGraph);
	void pushBackward(char *outGraph);
};

GRAPHPUSH::GRAPHPUSH(GRAPH &graph){
	p2NO=graph.p2NO;
	symbolList=graph.symbolList;
	symbolNum=graph.symbolNum;
	nodeNum=graph.nodeNum;
	terminal=graph.terminal;
	terminalNum=graph.terminalNum;
	machineType=graph.machineType;

	str2index(symbolList, symbolNum, EMPTY, epsIndex);
	str2index(symbolList, symbolNum, SIL, silIndex);
	str2index(symbolList, symbolNum, FSTR, fstrIndex);

	traverseNum=80000;
	tp = new TOPHONE[traverseNum];
	inDegree = new int[nodeNum];
	doMove = new int[nodeNum];
}

GRAPHPUSH::~GRAPHPUSH(){
	if(tp!=NULL){
		delete [] tp;
		tp=NULL;
	}
	if(inDegree!=NULL){
		delete [] inDegree;
		inDegree=NULL;
	}
	if(doMove!=NULL){
		delete [] doMove;
		doMove=NULL;
	}
}

void GRAPHPUSH::str2index(SYMBOLLIST *sl, int slNum, const char *str, int &strIndex){
	UTILITY util;
	strIndex=util.stringSearch(sl, 0, slNum-1, str);
	if(strIndex==-1){
		printf("Warning: %s not found.\n", str);
		exit(1);
	}
}

void GRAPHPUSH::getInDegree(){
	memset(inDegree, 0, sizeof(int)*nodeNum);
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<p2NO[i]->branchNum;j++)
			inDegree[p2NO[i]->arc[j].endNode]++;
	}
}

bool GRAPHPUSH::nodeProceed(int node){
	UTILITY util;
	bool result=false;
	if(inDegree[node]==1){
		result=true;
		// check its outlabel(s)
		for(int i=0;i<p2NO[node]->branchNum;i++){
			if( (p2NO[node]->arc[i].out!=epsIndex)||(p2NO[node]->arc[i].in==silIndex)){
				result=false;
				break;
			}
		}
	}	

	return result;
}

void GRAPHPUSH::showPath(int node){
	int tmpNode;
	printf("From %d, %d paths\n", node, pathNum);
	for(int i=0;i<pathNum;i++){
		tmpNode=node;
		printf("\t%d arcs ", tp[i].arcNum);
		for(int j=0;j<tp[i].arcNum;j++){
			printf("%d.", tp[i].arc[j]);
			printf("[%d] ", p2NO[tmpNode]->arc[tp[i].arc[j]].endNode);
			tmpNode=p2NO[tmpNode]->arc[tp[i].arc[j]].endNode;
		}
		printf("\n");
	}
}

void GRAPHPUSH::addTransition(int current, int branch){
	// assign
	tp[pathNum].dstNode=current;
	tp[pathNum].dstBranch=branch;

	tp[pathNum].arc[tp[pathNum].arcNum]=branch;
	tp[pathNum].arcNum++;

	tp[pathNum].endNode=p2NO[current]->arc[branch].endNode;
	if(tp[pathNum].arcNum>traverseDepth){
		printf("Transition number exceeds traverseDepth %d:\n", traverseDepth);
		printf("an infinite loop or phone sequence too long\n");
		exit(1);
	}
	if(pathNum+1>traverseNum){
		printf("Node %d, branch %d: pathNum %d exceeds traverseNum %d\n", current, branch, pathNum, traverseNum);
		exit(1);
	}
}

bool GRAPHPUSH::reachWordBoundary(int node, int branch){
	UTILITY util;
	bool cond1, cond2, cond3, reach;
	(util.isTerminal(terminal, terminalNum, node)==true)?cond1=true:cond1=false;
	(inDegree[p2NO[node]->arc[branch].endNode]>1)?cond2=true:cond2=false;
	(p2NO[node]->arc[branch].out!=epsIndex)?cond3=true:cond3=false; // seems redundant, see nodeProceed()

	(cond1||cond2||cond3)?reach=true:reach=false;

	return reach;
}

void GRAPHPUSH::initialize(){
	pathNum=0;
	tp[0].arcNum=0;
}

void GRAPHPUSH::toWordBoundary(int node){
	if(nodeProceed(node)==true){
		if(DEBUG_MODE) printf("\tnode %d Proceed=TRUE\n", node);
		for(int i=0;i<p2NO[node]->branchNum;i++){
			addTransition(node, i);
			if(reachWordBoundary(node, i)==false){
				if(DEBUG_MODE) printf("\t\tn:%d reachWB=FALSE\n", node);
				toWordBoundary(p2NO[node]->arc[i].endNode);
			}else{
				if(DEBUG_MODE) printf("\t\tn:%d reachWB=TRUE\n", node);
				memcpy(tp[pathNum+1].arc, tp[pathNum].arc, sizeof(int)*tp[pathNum].arcNum);
				pathNum++;
				if(DEBUG_MODE) printf("\tpathNum=%d\n", pathNum);
				tp[pathNum].arcNum=0; //???
			}
		}
	}else{
		if( tp[pathNum].arcNum > 0 )
			pathNum++;
	}
	
}

void GRAPHPUSH::moveOutlabelandWeight(int node, int branch){
	int endNode, s, t, srcLabel=p2NO[node]->arc[branch].out;
	float tempWeight, srcWeight=p2NO[node]->arc[branch].weight;

  // in some cases, 'pathNum' is equal to 0
	for(int p=0;p<pathNum;p++){
		tempWeight=0;
		endNode=p2NO[node]->arc[branch].endNode;
		for(int q=0;q<tp[p].arcNum;q++){
			tempWeight+=p2NO[endNode]->arc[tp[p].arc[q]].weight;
			p2NO[endNode]->arc[tp[p].arc[q]].weight=0;	// reset weight
			endNode=p2NO[endNode]->arc[tp[p].arc[q]].endNode;
		}
		s=tp[p].dstNode;
		t=tp[p].dstBranch;
		p2NO[s]->arc[t].weight=tempWeight+srcWeight;
		p2NO[s]->arc[t].out=srcLabel;
		// reset
		// the original information has been copied to 'srcLabel' and 'srcWeight', so it's safe to reset them
		p2NO[node]->arc[branch].out=epsIndex;
		p2NO[node]->arc[branch].weight=0;
	}
}

void GRAPHPUSH::pushForward(char *outGraph){
	// retrieve the number of inputs of each node
	getInDegree();

	int outSym, endNode;

	// the places need to move label/weight
	memset(doMove, 0, sizeof(int)*nodeNum);
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<p2NO[i]->branchNum;j++){
			endNode=p2NO[i]->arc[j].endNode;
			outSym=p2NO[i]->arc[j].out;
			// if output label is not equal to <eps>, <F>, or <sil>, mark it for future processing
			if((outSym!=epsIndex)&&(outSym!=fstrIndex)&&(p2NO[i]->arc[j].in!=silIndex)){
				doMove[i]=1;
				break;
			}
		}
	}
	// move
	for(int i=0;i<nodeNum;i++){
		if(doMove[i]==1){
			if(DEBUG_MODE) printf("node %d doMove=1\n", i);
			// check all branches of i-th node
			for(int j=0;j<p2NO[i]->branchNum;j++){
				endNode=p2NO[i]->arc[j].endNode;
				outSym=p2NO[i]->arc[j].out;
				// if output label is not equal to <eps>, <F>, or <sil>, perform the push
				if((outSym!=epsIndex)&&(outSym!=fstrIndex)&&(p2NO[i]->arc[j].in!=silIndex)){
					initialize();
					toWordBoundary(endNode);
					if(DEBUG_MODE) showPath(endNode);
					moveOutlabelandWeight(i, j);
				}
			}
		}
	}

	UTILITY util;
	util.outputGraph(p2NO, nodeNum, terminal, terminalNum, symbolList, machineType, outGraph);
}


int main(int argc, char **argv){
	if(argc!=4){
		printf("%s inGraph outGraph sym\n", argv[0]);
		printf("push forward the weights and outlabels\n");
		exit(1);
	}
	char inGraph[MAXLEN], outGraph[MAXLEN], symFile[MAXLEN];

	strcpy(inGraph, argv[1]);
	strcpy(outGraph, argv[2]);
	strcpy(symFile, argv[3]);

	GRAPH graph;
	graph.readGraph(inGraph, symFile, 0);

	GRAPHPUSH gp(graph);
	gp.pushForward(outGraph);
	
	return 0;
}
