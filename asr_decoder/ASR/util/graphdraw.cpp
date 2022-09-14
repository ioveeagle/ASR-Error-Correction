#include "recog.h"

class GRAPHDRAW {
    private:
        int uniCount;
		int *uniGram;
    public:
        GRAPHDRAW(GRAPH &G);
        ~GRAPHDRAW();
        void writeOutDot(char*, GRAPH&, MACRO&, int);
};

GRAPHDRAW::GRAPHDRAW(GRAPH &G){}
GRAPHDRAW::~GRAPHDRAW(){
	delete [] uniGram;
}

void GRAPHDRAW::writeOutDot(char *outFile, GRAPH &G, MACRO &M, int uniCount){
    FILE *fpOut;
    int i, endNode, tmpCount=0; 
    char inStr[MODELSTRLEN], outStr[WORDLEN];    
    uniGram = new int[G.nodeNum];
    for( i=0; i<G.nodeNum; i++ ) uniGram[i]=1;
	
    fpOut = fopen(outFile, "wt");
    if( fpOut!= NULL ){        
        fprintf(fpOut, "digraph g{\nedge [fontname=\"MSJH\"];\nrankdir=LR;\n");
		//fprintf(fpOut, "graph g{\nedge [fontname=\"MSJH\"];\nrankdir=LR;\n");
        for(i=0; i<G.nodeNum; i++){
			if( uniGram[i]<0 )
				continue;
			
            for(int j=0; j<G.p2N[i]->branchNum; j++){
                endNode = G.arc[G.p2N[i]->arcPos+j].endNode;
                fprintf(fpOut, "%d->%d ",i,endNode);
				//fprintf(fpOut, "%d--%d ",i,endNode);

                G.arc[G.p2N[i]->arcPos+j].in==-1?strcpy(inStr, "<eps>"):strcpy(inStr, M.p2MMF[G.arc[G.p2N[i]->arcPos+j].in]->str);
                
                if( strcmp(G.symbolList[G.arc[G.p2N[i]->arcPos+j].out].str, "<eps>")==0 ){
                    strcpy(outStr, "<eps>");
                }else{
                    strcpy(outStr, G.symbolList[G.arc[G.p2N[i]->arcPos+j].out].str);
					if( strcmp(outStr, "<F>")!=0 ){
						uniGram[endNode]=-1;
						tmpCount++;
						if( endNode==5 ) printf("i=%d j=%d %s %s %d\n", i, j, inStr, outStr, G.arc[G.p2N[i]->arcPos+j].out);
					}
                }

                fprintf(fpOut, "[label=\"%s:%s\"];\n", inStr, outStr);
            }
            if( tmpCount>=uniCount ) 
                break;
        }
        fprintf(fpOut, "}\n");
    }
    fclose(fpOut);
}
int main(int argc, char **argv){
	if(argc!=6){
		printf("Usage: %s graph macro sym unigramCount dotFormatFile\n", argv[0]);
		printf("produce DOT formatted text file.\n\n");
        printf("For small graphs, use the following command to generate PDF files:\n>dot -Tpdf -O dotFormatFile\n");
        printf("For very large graphs, use software 'ZGRviewer' instead.\n");
		exit(1);
	}	

	char graphFile[MAXLEN], symFile[MAXLEN], outFile[MAXLEN], macroFile[MAXLEN], unigramCount[MAXLEN];    
	strcpy(graphFile, argv[1]);
    strcpy(macroFile, argv[2]);
	strcpy(symFile, argv[3]);
    strcpy(unigramCount, argv[4]);
    strcpy(outFile, argv[5]);    

    GRAPH graph;
    MACRO macro;
    graph.readGraph2Decode(graphFile, symFile, 1.0, 0);
	macro.readMacroBin(macroFile);

    printf("%s has Node:%d Arc:%d\n", graphFile, graph.nodeNum, graph.arcNum);    

    GRAPHDRAW gDraw(graph);    
    gDraw.writeOutDot(outFile, graph, macro, atoi(unigramCount));

    return 0;
}

