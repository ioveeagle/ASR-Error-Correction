#include "recog.h"

#define traverseDepth	50
#define DEBUG_MODE 1
#define SMALL -999999999.9f

class GRAPHPUSHBACK {
private:
	NODE_OFFLINE **p2NO;
	SYMBOLLIST *symbolList;
	int symbolNum;
	int epsIndex, silIndex, fstrIndex;
	int nodeNum;
	int machineType;
	int *terminal;
	int terminalNum;
	//int *inDegree;
	int *doReview;
	float	*pushBackLangWeight;
	FILE *debugfp;
	//void getInDegree();
	void str2index(SYMBOLLIST *sl, int slNum, const char *str, int &strIndex);
	float landWeightPushBack(int node, int branch);
	void outputGraph(char *outGraph);
public:
	GRAPHPUSHBACK(GRAPH &graph);
	~GRAPHPUSHBACK();
	void pushBackward(char *outGraph);
	void markReviewNode();
	void attachgraph(GRAPH &graph);
	void freegraph(GRAPH &graph);
};

void GRAPHPUSHBACK::freegraph(GRAPH &graph)
{
	if(graph.NO!=NULL){
		delete [] graph.NO;
		graph.NO=NULL;
	}
	if(graph.p2NO!=NULL){
		delete [] graph.p2NO;
		graph.p2NO=NULL;
	}
	if(graph.N!=NULL){
		delete [] graph.N;
		graph.N=NULL;
	}
	if(graph.p2N!=NULL){
		delete [] graph.p2N;
		graph.p2N=NULL;
	}
	if(graph.arc!=NULL){
		delete [] graph.arc;
		graph.arc=NULL;
	}
	if(graph.terminal!=NULL){
		delete [] graph.terminal;
		graph.terminal=NULL;
	}
	if(graph.symbolList!=NULL){
		delete [] graph.symbolList;
		graph.symbolList=NULL;
	}
}

void GRAPHPUSHBACK::attachgraph(GRAPH &graph)
{
	p2NO=graph.p2NO;
	symbolList=graph.symbolList;
	symbolNum=graph.symbolNum;
	nodeNum=graph.nodeNum;
	terminal=graph.terminal;
	terminalNum=graph.terminalNum;
	machineType=graph.machineType;
}

GRAPHPUSHBACK::GRAPHPUSHBACK(GRAPH &graph){
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
	/*
	printf("EMPTY %s code %d\n",EMPTY, epsIndex);
	printf("SIL %s code %d\n",SIL, silIndex);
	printf("FSTR %s code %d\n",FSTR, fstrIndex);
	getchar();
	
	FILE *fp=fopen("symlist.txt","wt");
	for(int i=0;i<symbolNum;i++)
		fprintf(fp,"[%d] %s\n",i,symbolList[i].str);
	fclose(fp);
	*/
	//inDegree = new int[nodeNum];
	pushBackLangWeight = new float[nodeNum];
	doReview = new int[nodeNum];
}

GRAPHPUSHBACK::~GRAPHPUSHBACK(){
	//if(inDegree!=NULL){
	//	delete [] inDegree;
	//	inDegree=NULL;
	//}
	if(pushBackLangWeight!=NULL){
		delete [] pushBackLangWeight;
		pushBackLangWeight=NULL;
	}
	if(doReview!=NULL){
		delete [] doReview;
		doReview=NULL;
	}
}

void GRAPHPUSHBACK::str2index(SYMBOLLIST *sl, int slNum, const char *str, int &strIndex){
	UTILITY util;
	strIndex=util.stringSearch(sl, 0, slNum-1, str);
	if(strIndex==-1){
		printf("Warning: %s not found.\n", str);
		exit(1);
	}
}
/*
void GRAPHPUSHBACK::getInDegree(){
	memset(inDegree, 0, sizeof(int)*nodeNum);
	for(int i=0;i<nodeNum;i++){
		for(int j=0;j<p2NO[i]->branchNum;j++)
			inDegree[p2NO[i]->arc[j].endNode]++;
		pushBackLangWeight[i]=SMALL;
	}
}
*/
float GRAPHPUSHBACK::landWeightPushBack(int node, int branch)
{
	int	endNode=p2NO[node]->arc[branch].endNode;
	
	if(doReview[endNode])
	{
		printf("Exception!! arc:%d	%d attach the word-end node and can't find the weight.\n",node,endNode);
		if(DEBUG_MODE) fclose(debugfp);
		exit(1);
	}
	
	if(pushBackLangWeight[endNode] != SMALL)
		return(pushBackLangWeight[endNode]);
	/*
	if(inDegree[endNode] > 1)
	{
		printf("Exception occurred!!\n");
		printf("landWeightPushBack(%d,%d) encountered the multi-in node\n",node,branch);
		if(DEBUG_MODE) fclose(debugfp);
		exit(1);
	}
	*/
	int outSym, inSym;
	float	maxWeight=SMALL;
	int	branchNum=p2NO[endNode]->branchNum;
	if(DEBUG_MODE) fprintf(debugfp,"Go node:%d branchnum:%d\n",endNode,branchNum);
	//printf("Go node:%d branchnum:%d\n",endNode,branchNum);
	for(int i=0;i < branchNum;i++)
	{
		inSym=p2NO[endNode]->arc[i].in;
		outSym=p2NO[endNode]->arc[i].out;
		//printf("%d %d %s:%s %f ",endNode,p2NO[endNode]->arc[i].endNode,symbolList[inSym].str,symbolList[outSym].str,p2NO[endNode]->arc[i].weight);
		// if output label is equal to <F> or input label is equal to <sil> or <eps>, no push back
		if((outSym==fstrIndex)||(inSym==silIndex)||(inSym==epsIndex))
		{
			//printf("inSym %d %s outSym %d %s\n",inSym, symbolList[inSym].str, outSym, symbolList[outSym].str);
			//printf("continue\n");
			continue;
		}
		if(p2NO[endNode]->arc[i].weight == 0)
		{
			if(DEBUG_MODE) fprintf(debugfp,"=>forward to arc:%d	%d\n",endNode,p2NO[endNode]->arc[i].endNode);
			p2NO[endNode]->arc[i].weight=landWeightPushBack(endNode,i);
			if(DEBUG_MODE) fprintf(debugfp,"<=end forward arc:%d	%d weight:%f\n",endNode,p2NO[endNode]->arc[i].endNode,p2NO[endNode]->arc[i].weight);
		}
		if(maxWeight < p2NO[endNode]->arc[i].weight)
			maxWeight=p2NO[endNode]->arc[i].weight;
		//printf("maxWeight %f\n",maxWeight);
	}
	
	for(int i=0;i < branchNum;i++)
	{
		p2NO[endNode]->arc[i].weight -= maxWeight;
	}
	
	//printf("Return maxWeight %f ",maxWeight);getchar();
	if(maxWeight == SMALL)
	{
		printf("Exception! maxWeight == SMALL\n");getchar();
	}
	
	pushBackLangWeight[endNode]=maxWeight;
	
	return(maxWeight);
}

void GRAPHPUSHBACK::pushBackward(char *outGraph)
{
	if(DEBUG_MODE) debugfp=fopen("graphpush2head_debug.txt","wt");
	
	//get the sentence begin node
	int i, j, inSym, outSym, node_1=-1;
	bool	found=false;	
	for(i=0;i<nodeNum;i++)
	{
		for(j=0;j<p2NO[i]->branchNum;j++)
		{
			inSym=p2NO[i]->arc[j].in;
			outSym=p2NO[i]->arc[j].out;
			//find the sentence begin node, the start node of unigram-1.
			if((inSym==epsIndex)&&(outSym==epsIndex))
			{
				node_1=p2NO[i]->arc[j].endNode;
				found=true;
				break;
			}
		}
		if(found)
			break;
	}
	if(!found)
	{
		if(DEBUG_MODE) fprintf(debugfp,"Can't find the sentence begin node. No <eps>:<eps> arc in graph!\n");
		if(DEBUG_MODE) fclose(debugfp);
		printf("Can't find the sentence begin node. No <eps>:<eps> arc in graph!\n");
		exit(1);
	}

	//get the backoff node of unigram, the start node of unigram-2.
	int current=node_1;
	found=false;
	for(j=0;j<p2NO[current]->branchNum;j++)
	{
		inSym=p2NO[current]->arc[j].in;
		outSym=p2NO[current]->arc[j].out;
		//find the first backoff node.
		if((inSym==epsIndex)&&(outSym==fstrIndex))
		{
			current=p2NO[current]->arc[j].endNode;
			found=true;
			break;
		}
	}
	if(!found)
	{
		if(DEBUG_MODE) fprintf(debugfp,"Can't find the first backoff node. No <eps>:<F> arc attached to the sentence begin node!\n");
		if(DEBUG_MODE) fclose(debugfp);
		printf("Can't find the first backoff node. No <eps>:<F> arc attached to the sentence begin node!\n");
		exit(1);
	}
	
	int node_2=-1;
	found=false;
	for(j=0;j<p2NO[current]->branchNum;j++)
	{
		inSym=p2NO[current]->arc[j].in;
		outSym=p2NO[current]->arc[j].out;
		//find the second backoff node.
		if((inSym==epsIndex)&&(outSym==fstrIndex))
		{
			node_2=p2NO[current]->arc[j].endNode;
			found=true;
			break;
		}
	}
	if(!found)
	{
		if(DEBUG_MODE) fprintf(debugfp,"Can't find the second backoff node. No <eps>:<F> arc attached to the first backoff node!\n");
		if(DEBUG_MODE) fclose(debugfp);
		printf("Can't find the second backoff node. No <eps>:<F> arc attached to the first backoff node!\n");
		exit(1);
	}
	
	//getInDegree();
	
	//----------
	//do review	
	//----------
	//Review node_1
	if(DEBUG_MODE) fprintf(debugfp,"Review node-1: %d\n",node_1);
	//printf("Review node-1: %d\n",node_1);
	current=node_1;
	//check the branch with zero weight
	for(j=0;j<p2NO[current]->branchNum;j++)
	{
		outSym=p2NO[current]->arc[j].out;
		inSym=p2NO[current]->arc[j].in;
		// if output label is equal to <F> or input label is equal to <sil> or <eps>, no push back
		if((outSym==fstrIndex)||(inSym==silIndex)||(inSym==epsIndex))
			continue;
		if(p2NO[current]->arc[j].weight == 0)
		{
			if(DEBUG_MODE) fprintf(debugfp,"zero arc:%d	%d\n",current,j);
			p2NO[current]->arc[j].weight=landWeightPushBack(current,j);
			if(DEBUG_MODE) fprintf(debugfp,"zero arc:%d	%d with weight %f\n",current,j,p2NO[current]->arc[j].weight);
		}
	}
	
	//Review node_2
	if(DEBUG_MODE) fprintf(debugfp,"Review node-2: %d\n",node_2);
	//printf("Review node-2: %d\n",node_2);
	current=node_2;
	//check the branch with zero weight
	for(j=0;j<p2NO[current]->branchNum;j++)
	{
		outSym=p2NO[current]->arc[j].out;
		inSym=p2NO[current]->arc[j].in;
		// if output label is equal to <F> or input label is equal to <sil> or <eps>, no push back
		if((outSym==fstrIndex)||(inSym==silIndex)||(inSym==epsIndex))
			continue;
		if(p2NO[current]->arc[j].weight == 0)
		{
			if(DEBUG_MODE) fprintf(debugfp,"zero arc:%d	%d\n",current,j);
			p2NO[current]->arc[j].weight=landWeightPushBack(current,j);
			if(DEBUG_MODE) fprintf(debugfp,"zero arc:%d	%d with weight %f\n",current,j,p2NO[current]->arc[j].weight);
		}
	}
	
	//Review the doReview[] nodes
	for(int i=0;i<nodeNum;i++)
	{
		if(!doReview[i])
			continue;
		current=i;
		if(DEBUG_MODE) fprintf(debugfp,"Review doReview node: %d\n",current);
		//printf("Review doReview node: %d\n",current);
		//check the branch with zero weight
		for(j=0;j<p2NO[current]->branchNum;j++)
		{
			outSym=p2NO[current]->arc[j].out;
			inSym=p2NO[current]->arc[j].in;
			// if output label is equal to <F> or input label is equal to <sil> or <eps>, no push back
			if((outSym==fstrIndex)||(inSym==silIndex)||(inSym==epsIndex))
				continue;
			if(p2NO[current]->arc[j].weight == 0)
			{
				if(DEBUG_MODE) fprintf(debugfp,"zero arc:%d	%d\n",current,j);
				p2NO[current]->arc[j].weight=landWeightPushBack(current,j);
				if(DEBUG_MODE) fprintf(debugfp,"zero arc:%d	%d with weight %f\n",current,j,p2NO[current]->arc[j].weight);
			}
		}
	}

	if(DEBUG_MODE) fclose(debugfp);

	UTILITY util;
	util.outputGraph(p2NO, nodeNum, terminal, terminalNum, symbolList, machineType, outGraph);
}

void GRAPHPUSHBACK::markReviewNode()
{
	int outSym, endNode;

	// the places need to review
	memset(doReview, 0, sizeof(int)*nodeNum);
	for(int i=0;i<nodeNum;i++)
	{
		for(int j=0;j<p2NO[i]->branchNum;j++)
		{
			endNode=p2NO[i]->arc[j].endNode;
			if(doReview[endNode])//already marked.
				continue;
			outSym=p2NO[i]->arc[j].out;
			//if output label is not equal to <eps> or <F>, or input label is not equal to <sil>, mark the endNode for review processing.
			if((outSym!=epsIndex)&&(outSym!=fstrIndex)&&(p2NO[i]->arc[j].in!=silIndex))
			{
				doReview[endNode]=1;
			}
		}
		pushBackLangWeight[i]=SMALL;
	}
}

int main(int argc, char **argv){
	if(argc!=5){
		printf("%s inGraphWithoutGraphPush inGraphWithGraphPush outGraph sym\n", argv[0]);
		printf("push the weight to the first phone of word\n");
		printf("inGraphWithoutGraphPush: the graph after graphreverse->graphreverse\n");
		printf("inGraphWithGraphPush: the graph after graphreverse->graphpush->graphreverse\n");
		exit(1);
	}
	char inGraphWithoutGraphPush[MAXLEN], inGraphWithGraphPush[MAXLEN], outGraph[MAXLEN], symFile[MAXLEN];

	strcpy(inGraphWithoutGraphPush, argv[1]);
	strcpy(inGraphWithGraphPush, argv[2]);
	strcpy(outGraph, argv[3]);
	strcpy(symFile, argv[4]);

	GRAPH graph;
	graph.readGraph(inGraphWithoutGraphPush, symFile, 0);

	GRAPHPUSHBACK gp(graph);
	gp.markReviewNode();
	
	gp.freegraph(graph);
	
	//GRAPH graph;
	graph.readGraph(inGraphWithGraphPush, symFile, 0);
	gp.attachgraph(graph);
	
	gp.pushBackward(outGraph);
	
	//printf("done\n");
	return 0;
}
