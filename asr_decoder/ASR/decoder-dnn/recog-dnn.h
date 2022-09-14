#ifndef RECOG_DNN_H
#define RECOG_DNN_H

//#define USE_LATTICE  // use lattice or not

#ifdef X64
#define XINT long
#else
#define XINT int
#endif

#ifdef ASR_AVX
#define ALIGN_BYTE	32
#define ALIGNSIZE	32
#else
#define ALIGN_BYTE	16
#define ALIGNSIZE	16
//#define ALIGNTYPE	__declspec(align(ALIGNSIZE))	// for VC
#endif
//#define ALIGN_BYTE	__attribute__(aligned(ALIGN_BYTE))	// for GCC

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <float.h> // Added by Chi-Yueh Lin @20110804

#include <omp.h>
#if defined(ASR_SSE2)
#include <emmintrin.h>	// SSE2: 128-bit
#elif defined(ASR_AVX)
#include <immintrin.h>	// AVX: 256-bit
#elif defined(ASR_NEON)
#include <arm_neon.h>
#endif


#define MAXLEN	2000		// Max length reading a line
#define MODELSTRLEN	32 // should match DNNSTRLEN
#define WORDLEN	50 // 50:default 100:LiveABC
#define MAX_STATE_NUM 8
#define MAX_PHONE_NUM 60 // 80:LiveABC
#define MAX_FRAME_NUM 12000 // 1000 // 2000:HUB4NE Inside Test
#define MAX_HISTORY_LEN	12000 // 300 for word history, 12000 (2 min) for frame history
#define DIC_OFFSET	100
#define EMPTY "<eps>"
#define SIL	"sil"
#define FSTR "<F>"
#define GP "GP"
#define SP "sp"
#define DNNSTRLEN 32

#ifdef USE_LATTICE
#define LATTICE_LINE 100000
#define LATTICE_LINE_LEN 100
#endif

#if defined(ASR_FIXED)
#define SCORETYPE int //@fixed
#else
#define SCORETYPE float //@float
#endif

typedef struct {
	float mixtureWeight;
	float *mean;
	float *variance;	
	float gconst;
} MIXTURE;

typedef struct {
	char str[MODELSTRLEN];	// used for sorting
	int mixtureNum;
	MIXTURE *mixture;
} TIEDSTATE;

typedef struct {
	char str[MODELSTRLEN];	// used for sorting
	int stateNum;
	float *transProb;
} TIEDMATRIX;

typedef struct {
	char str[MODELSTRLEN];	// used for sorting
	int mixtureNum;
	SCORETYPE *mixtureWeight;
	SCORETYPE *mean;
	SCORETYPE *variance;
	SCORETYPE *gconst;
} TIEDSTATEVEC;				// vectorized for decoding

typedef struct{
	char str[MODELSTRLEN];	// used for sorting
	int stateNum;
	int *tiedStateIndex;
	int tiedMatrixIndex;
} MASTERMACROFILE;

typedef struct {
	char str[WORDLEN];
} SYMBOLLIST;

#ifdef USE_LATTICE
typedef struct {
	char str[WORDLEN*3];
} LATTICELIST;
#endif

typedef struct {
	char transcription[MAXLEN];
	char speaker[50];
	char feaFile[MAXLEN]; // 100
} FEALIST;

typedef struct {
	int node;
	int branch;
	int state;	
	int hist;
	int reDirectPath;
	SCORETYPE score;
	int phonePos;

	#ifdef USE_LATTICE
	SCORETYPE lmScore; // for lattice banco@20120712
	SCORETYPE amScore; // for lattice banco@20120712
	#endif
} ARENA;

typedef struct {
	int frameBoundary[MAX_HISTORY_LEN];
	int arc[MAX_HISTORY_LEN];
	int pathNum;
	int accessed;
	int frame;

	#ifdef USE_MINIMIZED_WFST
	short curOut;
	short stateOut;
	#endif

	#ifdef USE_LATTICE
	SCORETYPE lmScore[MAX_HISTORY_LEN]; // for lattice banco@20120712
	SCORETYPE amScore[MAX_HISTORY_LEN]; // for lattice banco@20120712
	#endif
} HISTORY;

typedef struct {
	int in;
	int out;
	float weight;
	int endNode;
} ARC;

typedef struct{
	int start;
	int end;
	int in;
	int out;
	float weight;
} LINK;

typedef struct {
	int branchNum;
	ARC *arc;
} NODE_OFFLINE;

typedef struct {
	int feaFile;
	int frame;
	int branchNum;
	int token;
	int arcPos;
} NODE;

typedef struct{
	int word;
	int phoneNum;
	int phone[MAX_PHONE_NUM];
} DIC;

typedef enum {
        AFFINE_TRANSFORM,
        SOFTMAX,
        PNORM,
        NORMALIZE,
	RELU,
	FIXEDSCALE,
        SUMGROUP,
	VQ_AFFINE_TRANSFORM
} LAYERTYPE;

typedef struct {
        float *W __attribute__ ((aligned(ALIGN_BYTE)));
        float *B __attribute__ ((aligned(ALIGN_BYTE)));
        int *G __attribute__ ((aligned(ALIGN_BYTE)));
	float *VQ __attribute__ ((aligned(ALIGN_BYTE)));
        int inDim;
        int outDim;
	int vqDim;
	int vqSize;
        LAYERTYPE type;
} DNNCOMPONENT;

typedef struct {
        char const *name;
        char const *biasname;
        float *W __attribute__ ((aligned(ALIGN_BYTE)));
        float *bias __attribute__ ((aligned(ALIGN_BYTE)));
        int inDim;
        int outDim;
} LSTMCOMPONENT;

typedef struct {
        char const *name;
        float *W __attribute__ ((aligned(ALIGN_BYTE)));
        int Dim;
} LSTM_SCALECOMPONENT;


class MACRO {
private:
	void allocateMem(char *);
public:
	MACRO();
	~MACRO();
	int modelNum;
	int streamWidth;
	int tsNum;
	int tmNum;
	TIEDSTATEVEC *ts;
	TIEDMATRIX *tm;
	TIEDSTATEVEC **p2ts;
	TIEDMATRIX **p2tm;
	MASTERMACROFILE **p2MMF;
	MASTERMACROFILE *MMF;
	void readMacroBin(char *);
};

class FEATURE {
private:
	void byteswap(void *, int);
	void FeaFieldInfo(char *);
	int frameWidth;
public:
	FEATURE(char *feaListFile, int vectorDim);
	~FEATURE();
	FEALIST *feaList;
	int feaListNum;	
	SCORETYPE *feaVec;
	int frameNum;
        void readMFCC(char *feaFileName, int vectorDim, int platform); // 0: floating point, 1: fixed point
        void readMFCCforDNN(char *feaFileName, int sizeBlock, int vectorDim, int platform); // 0: floating point, 1: fixed point
        void readMFCCforLSTM(char *feaFileName, int sizeBlock, int vectorDim, int labelDelay, int platform); // 0: floating point, 1: fixed point
};

class GRAPH {
private:
	void allocateMem(char *graphFile);
	int reDirectPathNum(char *);
	void splitPathString(int, int, char *);
	void saveNNInfoPartialExpand(char *str);
	void startEndInOutWeight(char *str);
	int obtainTerminal();
	int graphFileLineNum;
public:
	GRAPH();
	~GRAPH();
	int machineType; // 0: acceptor, 1: transducer
	NODE *N;
	NODE_OFFLINE *NO;
	NODE **p2N;
	NODE_OFFLINE **p2NO;
	ARC *arc;		
	int modelNum;
	int arcNum, nodeNum;
	int *terminal, terminalNum;
	int symbolNum;
	SYMBOLLIST *symbolList;
	void readGraph(char *graphFile, char *symListFile, int partialExpand); // partialExpand: 0=no, 1=yes
	void readGraph2Decode(char *graphFile, char *symListFile, float alpha, int platform); // 0: floating point, 1: fixed point
};

class UTILITY {
private:
	void makeStartFromZero(LINK **k, int lkNum);
	//void makeStartFromZero(LINK **k, unsigned int lkNum); // banco
	int maxNodeNumFromLink(LINK **k, int lkNum);
	//unsigned int maxNodeNumFromLink(LINK **k, unsigned int lkNum); //banco
	void sortLink(LINK **k, int lkNum);
	void outputTransition(int s, int e, int in, int out, float weight, SYMBOLLIST *sl, int mType, char *outStr);
	void sortDic(DIC *d, int left, int right);
	int partition(ARENA **p2arena, int deb, int fin);
	#ifdef USE_LATTICE
	int latLine;
	int strPos;
	char *memLattice;
	#endif
public:
	UTILITY();
	~UTILITY();
	void getGraphInfo(char *inFile, int &inFileLineNum, int &nodeNum, int &arcNum, int &machineType);
	void readSymbolList(SYMBOLLIST *sym, char *fileName, int symNum);
	void readDic(DIC *d, int &entryNum, SYMBOLLIST *sl, int slNum, char *inDic);
	int uniqueSymbol(SYMBOLLIST *sl, int slNum);
	FILE *openFile(const char *fileName, const char *mode);
	int getFileLineCount(char *);
	void assignParameter(char *graphFile, char *macroBinFile, char *feaListFile, char *lookupTableFile, char *symbolListFile, char *dicFile, int &beamSize, float &alphaValue, int argc, char **argv);
	int stringSearch(SYMBOLLIST *,int low, int up, const char *);
	int stringSearch(MASTERMACROFILE **,int low, int up, const char *);
	void sortString(SYMBOLLIST *, XINT, XINT);
	void pseudoSort(ARENA **p2arena, int low, int up, int topN);
	void quickSort(ARENA **p2arena,int low,int up);
	bool isNumeric(const char *);
	void *aligned_malloc(size_t, size_t);
	void aligned_free(void *);
	void representLink(LINK **k, NODE_OFFLINE **p, int nNum);	
	void nodeRenumerate(LINK **k, int lkNum, int *terminalNode, int terminalNodeNum);
	//void nodeRenumerate(LINK **k, unsigned int lkNum, int *terminalNode, int terminalNodeNum); // banco
	bool isTerminal(int *terminalNode, int terminalNodeNum, int node);
	void outputGraph(LINK **k, int lkNum, int *terminalNode, int terminalNodeNum, SYMBOLLIST *sym, int mType, char *outFile);
	//void outputGraph(LINK **k, unsigned int lkNum, int *terminalNode, int terminalNodeNum, SYMBOLLIST *sym, int mType, char *outFile); //banco
	void outputGraph(NODE_OFFLINE **p, int nNum, int *t, int tNum, SYMBOLLIST *sym, int mType, char *outFile);
	void writeVector(FILE *f, int *s, int num, int breakSize);
	void showArena(ARENA **p2a, HISTORY *h, GRAPH &g, MACRO &m, char *fileName, int showNum);


	#ifdef USE_LATTICE
	void showLatticeMem(ARENA **p2a, HISTORY *h, GRAPH &g, MACRO &m, char *fileName, int showNum, int frame, int type);

	void showLattice(ARENA **p2a, HISTORY *h, GRAPH &g, MACRO &m, char *fileName, int showNum, int frame, int type); // 20120813
	void showLattice2(ARENA **p2a, HISTORY *h, GRAPH &g, MACRO &m, char *fileName, int showNum, int frame, int type); // 20120813
	int uniqueSymbol(LATTICELIST *sl, int slNum);
	void sortString(LATTICELIST *, int, int);
	#endif
};

class DNN {
private:
	SCORETYPE **tmpOutputDNN __attribute__ ((aligned(ALIGN_BYTE)));
	SCORETYPE *fea __attribute__ ((aligned(ALIGN_BYTE)));
	SCORETYPE *buf_fea __attribute__ ((aligned(ALIGN_BYTE)));
	int pad_feature;
	UTILITY util;

	int initializeDNN(void);
public:
	DNN();
	//~DNN();
	TIEDMATRIX *tm;
	TIEDMATRIX **p2tm;
	MASTERMACROFILE *MMF;
	MASTERMACROFILE **p2MMF;

	DNNCOMPONENT *DNNAM;

	int numTriphone;
	int numTransitionMatrix;
	int numDnnComponent;
	int sizeBlock;
	int streamWidth;
	int numPrior;
	int applyLog;
	int applyPrior;

	SCORETYPE **frameScore __attribute__ ((aligned(ALIGN_BYTE)));
	SCORETYPE *Prior __attribute__ ((aligned(ALIGN_BYTE)));
	#if defined(LAZY)
	int *lazyStateMask __attribute__ ((aligned(ALIGN_BYTE)));
	int *lazyStateIdx __attribute__ ((aligned(ALIGN_BYTE)));
	int lastAffineLayer;
	int numLazyIdx;
	#endif

	int readMacroDNN(char *);
	#if defined(ASR_SSE2) || defined(ASR_AVX) || defined(ASR_NEON)
	void propagateSIMD(SCORETYPE *, int, int, int);
	#else
	void propagate(SCORETYPE *, int, int, int);
	#endif
	SCORETYPE* generateTestFeature(int, int, int, int);
	SCORETYPE* readTestFeature(char *, int *, int);
};

class LSTM {
private:
        //variable
        int INPUT_DIM, HIDDEN_DIM, Lstm1_r_t_dim;
        int component_num;
        LSTMCOMPONENT L0_lda;
        LSTMCOMPONENT Lstm1_W_c_xr;
        LSTMCOMPONENT Lstm1_W_f_xr;
        LSTMCOMPONENT Lstm1_W_i_xr;
        LSTMCOMPONENT Lstm1_W_o_xr;
        LSTMCOMPONENT Lstm1_W_m;
        LSTMCOMPONENT Final_Affine;
        LSTM_SCALECOMPONENT Lstm1_W_ic;
        LSTM_SCALECOMPONENT Lstm1_W_fc;
        LSTM_SCALECOMPONENT Lstm1_W_oc;
        LSTM_SCALECOMPONENT Priors;
        float *Lstm1_r_t, *Lstm1_r_t_pre;
        float *Lstm1_g1, *Lstm1_f1, *Lstm1_i1, *Lstm1_o1;
        float *Lstm1_f2, *Lstm1_i2, *Lstm1_o2;
        float *Lstm1_g_t, *Lstm1_c_t, *Lstm1_c_t_pre;
        float *Lstm1_i_t, *Lstm1_c1_t, *Lstm1_c2_t;
        float *Lstm1_f_t, *Lstm1_h_t, *Lstm1_o_t;
        float *L0_lda_out, *Lstm1_m_t, *output;

        //function
        void AffineTrans(float *out, float *weight_mat, float *bias, float *input, int mat_row, int mat_col);
        void Append(float *out, float *input1, float *input2, int input1_dim, int input2_dim);
        void hyper_tan(float *out, float *input, int dim);
        void sigmoid(float *out, float *input, int dim);
        void add(float *out, float *input1, float *input2, int dim);
        void dot_product(float *out, float *input1, float *input2, int dim);
        void softmax(float *out, float *input, int dim);
        void reset(float *input, int dim);
        void evaluateLSTM(SCORETYPE *pFeat, int startFrame, int numFrame, int numFeaDim, bool runSoftmax);
        void initialize(void);

	SCORETYPE *buf_fea __attribute__ ((aligned(ALIGN_BYTE)));
	UTILITY util;
public:
	TIEDMATRIX *tm;
	TIEDMATRIX **p2tm;
	MASTERMACROFILE *MMF;
	MASTERMACROFILE **p2MMF;

	SCORETYPE **frameScore __attribute__ ((aligned(ALIGN_BYTE)));

        int OUTPUT_DIM, numFrame, numFeaDim;
	int streamWidth;
	int sizeBlock;
	int numTriphone;

        bool applyLog, applyPrior;
	int applyDelayLabel, outputDelay, skipLSTM;
        int readMacroLSTM(char const *fname);
        void propagateSIMD(SCORETYPE *fea, int startFrame, int numFrame, int numFeaDim);
        LSTM();
        //~LSTM();
};


class SEARCH {
private:	
	ARENA *arena, **p2arena;
	HISTORY *hist;
	void save2arena(int type, int token, int &save2Pos, int node, int branch, int state, int hist, int reDirectPath, SCORETYPE score, int phonePos);
	void updateToken(int token, int node, int branch, int state, int hist, int reDirectPath, SCORETYPE score, int phonePos);	
	void getActiveToken();
	int move2Transition(int, int, int, int, int);
	void compete(int node, SCORETYPE score, int competeTokenIdx);
	void initialize();
	void selfState();
	void nextState();
	void competeAtNode();
	void updatestateCompete(int token);
	int reDirection(int, int, int);
	void accessHistory();
	void updateHistory();
	void updateHistoryFramewise();
	void resetGraphInfo(int option);	
	int obtainModel(int node, int branch, int phonePos);
	SCORETYPE getTokenScore(int node, int branch, int phonePos, int state, int calCase, SCORETYPE tokenScore, SCORETYPE LMscore);
	SCORETYPE stateScore(int);
	SCORETYPE streamScore(int tsIdx);
	int streamProbShift(int tsIdx);

	float transitionProb(int, int, int);
	double mixLogSum(double, double);
	void computemixLogSumLookupTable();
	void allocatestateCompete();
	void readDic(char *dicFile);

	#if defined(LAZY)
	/*
	void lazy_setStateMask();
	void lazy_reDirection(int, int, int);
	void lazy_move2Transition(int, int, int, int, int); 
	*/
	#endif

	DIC *dic;
	
	int dicEntryNum;
	int arenaSize;
	int feaFileIndex;
	int frame;
	int epsIndex;
	int fIndex; // banco@20120629
	int gpIndex, spIndex; // banco@20120924
	
	void readLookupTable(char *fileName, float alpha);
	unsigned short int *LTendNodeNum;
	//unsigned short int *LTendNodeArrayPosition;
	unsigned int *LTendNodeArrayPosition; // LVCSR
	int *LTendNodeArray;
	SCORETYPE *LTProb;
	
	int *stateCompete, *arc2stateCompete, stateCompeteCounter, arc2stateCompeteNum;
	unsigned int *arc2stateCompetePointer; // @20120723

	SCORETYPE *cacheScore;
	int beamSize;
	int tokenNum;
	int tmpTokenNum;

	#if defined(ASR_SSE2)
	__m128d mixLogSumLookupScale;	
	#elif defined(ASR_AVX)
	__m256d mixLogSumLookupScale;
	#else
	double mixLogSumLookupScale;
	#endif
	
	double *mixLogSumLookupTable;
	int mixLogSumLookupTableSize;

	UTILITY U;
	//MACRO M;
	#if defined(USE_LSTM)
	LSTM D;
	#else
	DNN D;
	#endif
	GRAPH G;
	SCORETYPE *p2feaVec;

	#ifdef USE_LATTICE
	SCORETYPE lmScore, tmpLmScore;
	SCORETYPE amScore, tmpAmScore;
	//char latticeBuffer[1000000];
	#endif 

public:
	//SEARCH(int keepTokenNum, float alpha, GRAPH &graph, MACRO &macro, char *lookupTableFile, char *dicFile);
	#if defined(USE_LSTM)
	SEARCH(int keepTokenNum, float alpha, GRAPH &graph, LSTM &macro, char *lookupTableFile, char *dicFile);
	#else
	SEARCH(int keepTokenNum, float alpha, GRAPH &graph, DNN &macro, char *lookupTableFile, char *dicFile);
	#endif
	~SEARCH();
	void doDecode(int keepBeamSize, int frameNum, SCORETYPE *feaVec, int fileIndex);
	void showResult(int topN, char *spk, char *feaFile);
	//int maxTokenNum;
};

#endif
