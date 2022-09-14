#include "recog.h"

#define IC_PENALTY_SCALE 100

class ICTABLE2HEADER{
private:
	int nodeNum, endNodeNum;
	// PC
	unsigned short int *LTendNodeNum;
	unsigned short int *LTendNodeArrayPosition;
	int *LTendNodeArray;
	float *LTProb;
	// IC
	int RDNodeNum;// redirect node num
	int *RDNode;
	int *RDEnd;
	int *RDWeight;
	void readLookupTable(char *fileName, float alpha);
public:
	ICTABLE2HEADER();
	~ICTABLE2HEADER();
	void doTable2Header(char *tableFile, char *headerFile);
};

ICTABLE2HEADER::ICTABLE2HEADER(){}

ICTABLE2HEADER::~ICTABLE2HEADER(){
	if(LTendNodeNum!=NULL){
		delete [] LTendNodeNum;
		LTendNodeNum=NULL;
	}
	if(LTendNodeArrayPosition!=NULL){
		delete [] LTendNodeArrayPosition;
		LTendNodeArrayPosition=NULL;
	}
	if(LTendNodeArray!=NULL){
		delete [] LTendNodeArray;
		LTendNodeArray=NULL;
	}
	if(LTProb!=NULL){
		delete [] LTProb;
		LTProb=NULL;
	}
	if(RDNode!=NULL){
		delete [] RDNode;
		RDNode=NULL;
	}
	if(RDEnd!=NULL){
		delete [] RDEnd;
		RDEnd=NULL;
	}
	if(RDWeight!=NULL){
		delete [] RDWeight;
		RDWeight=NULL;
	}
}

void ICTABLE2HEADER::readLookupTable(char *fileName, float alpha){
	size_t st;

	UTILITY util;
	FILE *out=util.openFile(fileName, "rb");
	int ary1Size, ary2Size, ary3Size, ary4Size;
	st=fread(&ary1Size, sizeof(int), 1, out);//number of endNodes
	st=fread(&ary2Size, sizeof(int), 1, out);//redirectNum of each node, represented in an array
	st=fread(&ary3Size, sizeof(int), 1, out);//set of endNodes
	st=fread(&ary4Size, sizeof(int), 1, out);//set of probs
	LTendNodeNum=new unsigned short int[ary1Size];
	LTendNodeArrayPosition=new unsigned short int[ary2Size];
	LTendNodeArray=new int[ary3Size];
	LTProb=new float[ary4Size];
	st=fread(LTendNodeNum, sizeof(unsigned short int), ary1Size, out);
	st=fread(LTendNodeArrayPosition, sizeof(unsigned short int), ary2Size, out);
	st=fread(LTendNodeArray, sizeof(int), ary3Size, out);
	st=fread(LTProb, sizeof(float), ary4Size, out);
	for(int i=0;i<ary4Size;i++)
		LTProb[i]=LTProb[i]*alpha;
	fclose(out);

	nodeNum=ary1Size;
	endNodeNum=ary3Size;
}

void ICTABLE2HEADER::doTable2Header(char *tableFile, char *headerFile){
	// read table
	readLookupTable(tableFile, 1);
	
	int c;
	// RDNodeNum
	c=0;
	for(int i=0;i<nodeNum;i++){
		if(LTendNodeNum[i]>0) // only count nodes that have redirected paths
			c++;	
	}
	RDNodeNum=c;	
	// RDNode
	c=0;
	RDNode=new int[RDNodeNum*3];
	for(int i=0;i<nodeNum;i++){
		if(LTendNodeNum[i]){ // same as LTendNodeNum>0
			RDNode[c]=i; // node i has redirected paths
			RDNode[RDNodeNum*1+c]=LTendNodeNum[i]; // redirect path number for i-th node
			RDNode[RDNodeNum*2+c]=LTendNodeArrayPosition[i]; // position to access RDEndNode
			c++;
		}
	}
	// RDEnd
	RDEnd=new int[endNodeNum];
	for(int i=0;i<endNodeNum;i++)
		RDEnd[i]=LTendNodeArray[i];
	// RDWeight
	RDWeight=new int[endNodeNum];
	for(int i=0;i<endNodeNum;i++)
		RDWeight[i]=(int)floor(LTProb[i]*IC_PENALTY_SCALE+0.5);

	// output
	UTILITY util;
	FILE *out=util.openFile(headerFile, "wt");
	fprintf(out, "//---------- LOOKUPTABLE ----------\n");
	fprintf(out, "const short int RDNodeNum=%d;\n", RDNodeNum);
	// RDNode
	fprintf(out, "const short int RDNode[%d]={", RDNodeNum*3);
	util.writeVector(out, RDNode, RDNodeNum*3, 10);
	// RDEnd
	fprintf(out, "const short int RDEnd[%d]={", endNodeNum);
	util.writeVector(out, RDEnd, endNodeNum, 10);
	// RDWeight
	fprintf(out, "const short int RDWeight[%d]={", endNodeNum);
	util.writeVector(out, RDWeight, endNodeNum, 10);

	fclose(out);
}

int main(int argc, char **argv){
	if(argc!=3){
		printf("Usage: %s DEST.table table.h\n", argv[0]);
		exit(1);
	}

	ICTABLE2HEADER ICt2h;
	ICt2h.doTable2Header(argv[1], argv[2]);

	return 0;
};
