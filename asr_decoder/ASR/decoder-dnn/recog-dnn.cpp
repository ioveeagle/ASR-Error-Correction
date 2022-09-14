#include "recog-dnn.h"

int main(int argc, char **argv){
	UTILITY util;
	int platform=0; // 0=floating point, 1=fixed point
	int beamSize;
	float alphaValue=1;


	char graphFile[50], macroBinFile[50], feaListFile[50], lookupTableFile[50], symbolFile[50], dicFile[50];
	util.assignParameter(graphFile, macroBinFile, feaListFile, lookupTableFile, symbolFile, dicFile, beamSize, alphaValue, argc, argv);

	#if defined(USE_LSTM)
	//printf("Use LSTM\n");
	LSTM macro;
	macro.readMacroLSTM(macroBinFile);
	#else
	DNN macro;
	macro.readMacroDNN(macroBinFile);
	#endif

	GRAPH graph;
	graph.readGraph2Decode(graphFile, symbolFile, alphaValue, platform);

	FEATURE feature(feaListFile, macro.streamWidth);

	SEARCH search(beamSize, alphaValue, graph, macro, lookupTableFile, dicFile);
	//printf("Search network initialize ok\n");	

	clock_t a;
	for(int i=0;i<feature.feaListNum;i++){ // # of files to be recognized
		#if defined(USE_LSTM)
		macro.applyDelayLabel=5;
		macro.skipLSTM=0;
		feature.readMFCCforLSTM(feature.feaList[i].feaFile, macro.sizeBlock, macro.streamWidth, macro.applyDelayLabel, platform);
		#else
		feature.readMFCCforDNN(feature.feaList[i].feaFile, macro.sizeBlock, macro.streamWidth, platform);
		#endif

		a=clock();
		search.doDecode(beamSize, feature.frameNum, feature.feaVec, i);
		search.showResult(1, feature.feaList[i].speaker, feature.feaList[i].feaFile);

		double duree=(double)(clock()-a)/CLOCKS_PER_SEC;
		printf("\t%.2f sec, %d frames -> %.2f RT\n", duree, feature.frameNum, duree*100/feature.frameNum);
	}
	return 0;
}

