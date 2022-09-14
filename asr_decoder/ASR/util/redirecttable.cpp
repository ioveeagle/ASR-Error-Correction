#include "recog.h"

#define traverseDepth	30
#define traverseNum	30000

typedef struct {
	int arcNum;
	int endNode;
	int arc[traverseDepth];
} TOPHONE;

class TRAVERSETOPHONE{
private:
	GRAPH G;
	int validPathNum;
	int pathNum;
	int *nodeHistoryLength;
	TOPHONE *tp;
	int *epsNode;
	void checkEps();
	void selectPath();
	int existNonEpsilonTransition(int node);
	int existEpsilonTransition(int node);
	void showTraversePath(int node, int pathNum);
	void addTransition(int node, int branch);
	void toLeaf(int current);
	void toLeafInitialize();
	int nodeHistoryTable[traverseNum];
	int nodeHistoryTableCount;
public:
	TRAVERSETOPHONE();
	~TRAVERSETOPHONE();
	void makeTable(char *graphFile, char *symFile, char *tableFile);
};

TRAVERSETOPHONE::TRAVERSETOPHONE(){
	tp=NULL;
	nodeHistoryLength=NULL;
	epsNode=NULL;
}

TRAVERSETOPHONE::~TRAVERSETOPHONE(){
	if(tp!=NULL){
		delete [] tp;
		tp=NULL;
	}
	if(nodeHistoryLength!=NULL){
		delete [] nodeHistoryLength;
		nodeHistoryLength=NULL;
	}
	if(epsNode!=NULL){
		delete [] epsNode;
		epsNode=NULL;
	}
}

void TRAVERSETOPHONE::showTraversePath(int node, int pathNum){
	int tmpNode;
	printf(">> From [%d], %d paths\n", node, pathNum);
	for(int i=0;i<pathNum;i++){
		tmpNode=node;
		printf("%d arcs ", tp[i].arcNum);
		for(int j=0;j<tp[i].arcNum;j++){
			printf("%d.", tp[i].arc[j]);
			printf("[%d] ", G.arc[G.p2N[tmpNode]->arcPos+j].endNode);
			tmpNode=G.arc[G.p2N[tmpNode]->arcPos+j].endNode;
		}
		printf("\n");
	}
	printf("\n");
}

// 'current' is the node index
// 'branch' is the branch index of node 'current'
void TRAVERSETOPHONE::addTransition(int current, int branch){
	// record the branch index in 'tp[pathNum].arc[]'
	tp[pathNum].arc[nodeHistoryLength[current]]=branch;
	// record the # of arcs traversed so far in this path
	tp[pathNum].arcNum=nodeHistoryLength[current]+1;
	// record the next node to visit
	tp[pathNum].endNode=G.arc[G.p2N[current]->arcPos+branch].endNode;
	if(tp[pathNum].arcNum>traverseDepth){ // traverseDepth := 30
		printf("Transition number exceeds %d, Node:%d, branch:%d\n", traverseDepth, current, branch);
		printf("Path too long or a loop exists\n");
		exit(1);
	}
	if(pathNum+1>traverseNum){
		printf("pathNum exceeds %d\n", traverseNum);
		exit(1);
	}
}

// 'current' is node index
void TRAVERSETOPHONE::toLeaf(int current){ 
	int endNode;
	for(int i=0;i<G.p2N[current]->branchNum;i++){
		// input label of this arc is <eps>, which means that the arc can be omitted during the decoding
		if(G.arc[G.p2N[current]->arcPos+i].in==-1){ 
			nodeHistoryTable[nodeHistoryTableCount]=current;
			nodeHistoryTableCount++;
			// 'current': node index; 'i': branch index
			addTransition(current, i); 
			// next node index to visit
			endNode=G.arc[G.p2N[current]->arcPos+i].endNode; 
			// next node has a history. (how many arcs are visited so far when visiting this 'endNode')
			nodeHistoryLength[endNode]=tp[pathNum].arcNum;


			if( nodeHistoryTableCount > traverseNum ){
				printf("nodeHitoryTableCount is full! %d\n", traverseNum);
				exit(1);
			}

			if(existNonEpsilonTransition(endNode)==1){ // branches of 'endNode' have non-<eps> input labels
				tp[pathNum+1]=tp[pathNum]; // copy the history so far to create a new traverse path
				pathNum++; // a new path is created
				if(existEpsilonTransition(endNode)==1)
					toLeaf(endNode); // traverse further if this 'endNode' still has <eps> branches
			}
			else // all branches of 'endNode' have <eps> input labels, traverse further!
					toLeaf(endNode);
		}
	}
}

// if 'node' has non-<eps> input labels or 'node' is a terminal node, return TRUE
int TRAVERSETOPHONE::existNonEpsilonTransition(int node){
	int existNonEpsilon=0;
	for(int i=0;i<G.p2N[node]->branchNum;i++){
		if(G.arc[G.p2N[node]->arcPos+i].in!=-1){// non-<eps>
			existNonEpsilon=1;
			break;
		}
	}
	UTILITY util;
	if(util.isTerminal(G.terminal, G.terminalNum, node)==true)
		existNonEpsilon=1;
	return existNonEpsilon;
}

// perform opposite way to existNonEpsilonTransition()
int TRAVERSETOPHONE::existEpsilonTransition(int node){
	int existEpsilon=0;
	for(int i=0;i<G.p2N[node]->branchNum;i++){
		if(G.arc[G.p2N[node]->arcPos+i].in==-1){
			existEpsilon=1;
			break;
		}
	}
	return existEpsilon;
}

// Initialize for 'toLeaf', this function executes before processing each node
void TRAVERSETOPHONE::toLeafInitialize(){
	pathNum=0;
	for(int i=0;i<traverseNum;i++)
		tp[i].arcNum=0;	
	//for(int i=0;i<G.nodeNum;i++)
	//	nodeHistoryLength[i]=0;
	for(int i=0;i<nodeHistoryTableCount;i++)
		nodeHistoryLength[nodeHistoryTable[i]]=0;
	nodeHistoryTableCount=0;
}

// if the 'endNode' of the path 'tp[i]' is a terminal node, this path is not a valid path,
// therefore its records are erased
void TRAVERSETOPHONE::selectPath(){
	UTILITY util;
	validPathNum=pathNum;
	for(int i=0;i<pathNum;i++){
		if(util.isTerminal(G.terminal, G.terminalNum, tp[i].endNode)==true){
			tp[i].arcNum=0;
			validPathNum--;
		}
	}
}

// check whether a node has a branch with <eps> input label
void TRAVERSETOPHONE::checkEps(){
	memset(epsNode, 0, sizeof(int)*G.nodeNum);
	for(int i=0;i<G.nodeNum;i++){
		for(int j=0;j<G.p2N[i]->branchNum;j++){
			if(G.arc[G.p2N[i]->arcPos+j].in==-1){
				epsNode[i]=1; // i-th node has <eps> branches
				break;
			}
		}
	}
}

void TRAVERSETOPHONE::makeTable(char *graphFile, char *symFile, char *tableFile){
	G.readGraph2Decode(graphFile, symFile, 1, 0);
	tp = new TOPHONE[traverseNum]; // traverseNum := 30000 (paths)
	nodeHistoryLength = new int[G.nodeNum]; // each node has its own history
	epsNode = new int[G.nodeNum];

	checkEps(); // check whether nodes in the graph have <eps> branches

	for(int i=0; i<traverseNum; i++)
		nodeHistoryTable[i]=0;
	nodeHistoryTableCount=0;
	memset(nodeHistoryLength, 0, sizeof(int)*G.nodeNum);

	UTILITY util;
	FILE *out=util.openFile(tableFile,"wt");
	float probSum;
	char transitionPath[MAXLEN], branchStr[MAXLEN];
	int branch, dstNode;
	for(int i=0;i<G.nodeNum;i++){
		// i-th node has <eps> branches, track its succeeding <eps> branches till the leaf node
		if(epsNode[i]==1){ 
			toLeafInitialize();
			toLeaf(i); // find out all paths from i-th node to leaf nodes (have non-<eps> branches)
			selectPath(); // eliminate invalid paths (whose 'endNode' is a terminal node in the graph)
			//showTraversePath(i, pathNum);
			if(validPathNum==0) // this node doesn't have any path needs to be redirected
				fprintf(out, "0\n");
			else{
				fprintf(out, "%d ", validPathNum); // how many paths are derived from node 'i'
				for(int j=0;j<pathNum;j++){
					if(tp[j].arcNum>0){// get score, dstNode & transition path						
						strcpy(transitionPath, "");
						probSum=0; // accumulate probability
						dstNode=i;
						for(int k=0;k<tp[j].arcNum;k++){ // this path consists of 'tp[j].arcNum' arcs
							branch=tp[j].arc[k]; // branch index (related to the visited node)
							probSum+=log10(exp(G.arc[G.p2N[dstNode]->arcPos+branch].weight));
							dstNode=G.arc[G.p2N[dstNode]->arcPos+branch].endNode; // next node index in the path
							sprintf(branchStr, "%d.", branch); // which branch
							strcat(transitionPath, branchStr);
						}
						transitionPath[strlen(transitionPath)-1]='\0'; // overwrite the last dot "." symbol
						fprintf(out, "%d,%f,%s ", dstNode, probSum, transitionPath);
					}
				}
				fprintf(out,"\n");
			}
		} // i-th node has no <eps> branches, thus no need to redirect
		else
			fprintf(out, "0\n");
		//printf("%d / %d\n", i, graph.nodeNum);
	}
	fclose(out);
}

int main(int argc, char **argv){
	if(argc!=4){
		printf("Usage:%s graph.bin sym.bin table.txt\n", argv[0]);
		exit(1);
	}
	char graphFile[100], symFile[100], lookupTableFile[100];
	strcpy(graphFile, argv[1]);
	strcpy(symFile, argv[2]);
	strcpy(lookupTableFile, argv[3]);	

	TRAVERSETOPHONE T;
	T.makeTable(graphFile, symFile, lookupTableFile);
	
	return 0;
}
 
