#include "recog.h"

class GRAPHSTACK {
private:
	int machineType;
	int terminal;
	int *graphNodeNum;
	void getGraphInfo(int argc, char **argv);
public:
	GRAPHSTACK();
	~GRAPHSTACK();
	void doGraphStack(int argc, char **argv);
};

GRAPHSTACK::GRAPHSTACK(){}

GRAPHSTACK::~GRAPHSTACK(){
	if(graphNodeNum!=NULL)
		delete [] graphNodeNum;
}

void GRAPHSTACK::getGraphInfo(int argc, char **argv){
	UTILITY util;
	int lineNum, tmpType, nodeNum, arcNum;
	graphNodeNum=new int[argc-2];

	util.getGraphInfo(argv[1], lineNum, nodeNum, arcNum, machineType);
	for(int i=1;i<argc-1;i++){
		util.getGraphInfo(argv[i], lineNum, nodeNum, arcNum, tmpType);
		if(tmpType!=machineType){
			printf("Graph type not consistent, check %s\n", argv[i]);
			exit(1);
		}
		graphNodeNum[i-1]=nodeNum;
	}

	terminal=1; // 2 nodes added, counted from 1
	for(int i=0;i<argc-2;i++)
		terminal+=graphNodeNum[i];
}

void GRAPHSTACK::doGraphStack(int argc, char **argv){
	UTILITY util;

	getGraphInfo(argc, argv);
	
	int addedNum;
	char msg[MAXLEN], fileName[MAXLEN], *p1, *p2, *p3, *p4, *p5;
	FILE *out=util.openFile(argv[argc-1], "wt");
	for(int i=0;i<argc-2;i++){
		strcpy(fileName, argv[i+1]);
		// 0 to graph start
		(i==0)?addedNum=1:addedNum+=graphNodeNum[i-1];
		(machineType==1)?fprintf(out, "0\t%d\t%s\t%s\t\n", addedNum, FSTR, FSTR):fprintf(out, "0\t%d\t%s\t\n", addedNum, FSTR);

		int fileLineNum=util.getFileLineCount(fileName);
		FILE *in=util.openFile(fileName, "rt");
		for(int j=0;j<fileLineNum;j++){
			fgets(msg, MAXLEN, in);
			msg[strlen(msg)-1]='\0';
			p1=strtok(msg, "\t ");
			p2=strtok(NULL, "\t ");
			p3=strtok(NULL, "\t ");
			p4=strtok(NULL, "\t ");
			p5=strtok(NULL, "\t ");
			if(p2!=NULL)// not terminal
				fprintf(out, "%d\t%d\t%s\t", atoi(p1)+addedNum, atoi(p2)+addedNum, p3);
			else{
				if(machineType==1)
					fprintf(out, "%d\t%d\t%s\t%s\t", atoi(p1)+addedNum, terminal, FSTR, FSTR);
				else
					fprintf(out, "%d\t%d\t%s\t", atoi(p1)+addedNum, terminal, FSTR);
			}

			if(p4!=NULL)
					fprintf(out, "%s\t", p4);
			if(p5!=NULL)
				fprintf(out, "%s\t", p5);
			fprintf(out, "\n");
		}
		fclose(in);
	}
	fprintf(out, "%d\t\n", terminal);
	fclose(out);
}

int main(int argc, char **argv){
	if(argc<=3){
		printf("Usage: %s graph1 graph2 ... graph.out\n", argv[0]);
		printf("It stacks graphs into one search space by adding <F> or <F>:<F>.\n");
		exit(1);
	}

	GRAPHSTACK gs;
	gs.doGraphStack(argc, argv);

	return 0;
}
