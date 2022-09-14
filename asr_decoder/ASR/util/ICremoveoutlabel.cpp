#include "recog.h"

#define DELIMITER "@"

class REMOVEOUTLABEL{
private:
	void outputTransition(FILE *f, GRAPH &g, int node, int branch, char *pch);
public:
	REMOVEOUTLABEL();
	~REMOVEOUTLABEL();
	void doOutLabelRemoval(char *graphFile, char *outFile, char *symFile);
};

REMOVEOUTLABEL::REMOVEOUTLABEL(){
}

REMOVEOUTLABEL::~REMOVEOUTLABEL(){}

void REMOVEOUTLABEL::outputTransition(FILE *f, GRAPH &g, int node, int branch, char *pch){
	char inStr[MAXLEN], outStr[MAXLEN];
	strcpy(inStr, g.symbolList[g.p2NO[node]->arc[branch].in].str);

	if(pch==NULL)
		//strcpy(outStr, g.symbolList[g.p2NO[node]->arc[branch].out].str);
		strcpy(outStr, EMPTY);
	else{
		strcpy(outStr, DELIMITER);
		strcat(outStr, pch);
	}

	fprintf(f, "%d\t%d\t%s\t%s", node, g.p2NO[node]->arc[branch].endNode, inStr, outStr);
	if(g.p2NO[node]->arc[branch].weight!=0)
		fprintf(f, "\t%f\n", (float)log10((double)exp(g.p2NO[node]->arc[branch].weight)));
	else
		fprintf(f, "\n");
}

void REMOVEOUTLABEL::doOutLabelRemoval(char *graphFile, char *outFile, char *symFile){
	UTILITY util;

	GRAPH G;
	G.readGraph(graphFile, symFile, 0);

	if(G.machineType==0){
		printf("Outlabel-removal is performed on transducers\n");
		exit(1);
	}

	char msg[MAXLEN], *pch;
	FILE *out=util.openFile(outFile, "wt");
	for(int i=0;i<G.nodeNum;i++){
		for(int j=0;j<G.p2NO[i]->branchNum;j++){
			// manipulate outLabel
			strcpy(msg, G.symbolList[G.p2NO[i]->arc[j].out].str);
			pch=strtok(msg, DELIMITER);
			pch=strtok(NULL, DELIMITER);
			outputTransition(out, G, i, j, pch);
		}
	}
	for(int i=0;i<G.terminalNum;i++)
		fprintf(out, "%d\n", G.terminal[i]);
	fclose(out);
}

int main(int argc, char **argv){
	if(argc!=4){
		printf("USAGE: %s graph.in graph.out sym\n", argv[0]);
		printf("It removes word labels, keeping @# ones\n");
		exit(1);
	}

	REMOVEOUTLABEL rol;
	rol.doOutLabelRemoval(argv[1], argv[2], argv[3]);

	return 0;
}
