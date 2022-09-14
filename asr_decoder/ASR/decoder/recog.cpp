#include "recog.h"

int main(int argc, char **argv){
	UTILITY util;
	int platform=0; // 0=floating point, 1=fixed point
	int beamSize;
	float alphaValue=1;


	char graphFile[50], macroBinFile[50], feaListFile[50], lookupTableFile[50], symbolFile[50], dicFile[50];
	util.assignParameter(graphFile, macroBinFile, feaListFile, lookupTableFile, symbolFile, dicFile, beamSize, alphaValue, argc, argv);
	
	MACRO macro;
	macro.readMacroBin(macroBinFile);
	//printf("Read macro ok...modelNum=%d streamWidth=%d tsNum=%d tmNum=%d\n", macro.modelNum, macro.streamWidth, macro.tsNum, macro.tmNum);

	GRAPH graph;
	graph.readGraph2Decode(graphFile, symbolFile, alphaValue, platform);
	//printf("Read graph ok...N=%d A=%d S=%d\n", graph.nodeNum, graph.arcNum, graph.symbolNum);

	FEATURE feature(feaListFile, macro.streamWidth);
	//printf("Read feature ok\n");

	SEARCH search(beamSize, alphaValue, graph, macro, lookupTableFile, dicFile);
	//printf("Search network initialize ok\n");	

	clock_t a;
	for(int i=0;i<feature.feaListNum;i++){ // # of files to be recognized
	//for(int i=0;i<3;i++){ // # of files to be recognized
	
		feature.readMFCC(feature.feaList[i].feaFile, macro.streamWidth, platform);
		//printf("%3d:%s\n",i, feature.feaList[i].transcription);

		a=clock();
		
		search.doDecode(beamSize, feature.frameNum, feature.feaVec, i);
		search.showResult(1, feature.feaList[i].speaker, feature.feaList[i].feaFile);

		double duree=(double)(clock()-a)/CLOCKS_PER_SEC;

		printf("\t%.2f sec, %d frames -> %.2f RT\n", duree, feature.frameNum, duree*100/feature.frameNum);
	}
	return 0;
}

