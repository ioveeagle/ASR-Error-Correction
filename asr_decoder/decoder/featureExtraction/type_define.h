//#define DO_CMN_OR_MVN  1 //0: CMN, 1:MVN
#define DO_AVG         2//1:MA, 2:ARMA
#define ORDER_M        2
#define TRUE 1
#define FALSE 0

#define FFT_ORDER 256 //256:for 8K;      512:for 16K
#define FFT_ORDER_2 128 //128:for 8K;      256:for 16K
#define FILTERNO  24
#define CEP_DIM 12
#define OUT_DIM 39
#define DIM     26
#define INPUT   26
#define ALPHA 0.95 //preemphasis cofe
#define SAMPPERIOD 1250 //1250:for 8K  //625:for 16K
#define FRAME_SIZE 160 //160:for 8k,  320:for 16K
#define FRAME_SHIFT 80 //80:for 8k, 160:for 16K
#define MAX_FRM_NO  9000
#define MAX_SAMPLE_NO 720000  // 30sec, 240000:for 8k, 480000:for 16k
#define PI   3.14159265358979
#define DELTA 2
#define DDELTA 1
typedef float FEA_DATA[INPUT];
typedef float CEP_DATA[CEP_DIM+1];
typedef float OUT_DATA[OUT_DIM];

#define   SILENCE         0
#define   MODEL           410
#define   I_MODEL         100
#define   F_MODEL         38
#define   I_STATE         3
#define   F_STATE         5
#define   STATE           8   //I_STATE+F_STATE
#define   EXP_STATE       (STATE+1)
#define   I_MIX           10
#define   F_MIX           10
#define   S_MIX           64
#define   TOPN            5

#define   SMALL           -9999999.0f
#define   BIG             9999999.0f

#define MAX_BYTE_IN_LINE 256
#define MAX_BYTE_IN_WORD 32

#define   VFR_THRESHOLD   0.65

typedef struct Complex_
{
   double x;
   double y;
}COMPLEX;


typedef struct tag_tree
{
	int	index, post, left, phra;
	unsigned short	syl;
} TREE;

typedef struct tag_node
{
	int	index;
	float	L;
	unsigned short	syl;
	unsigned char	state;
} BEAMNODE;

typedef char LINE_STRING[MAX_BYTE_IN_LINE];

#define EC 0//0
#define LTA 0//0
#define RUNVFR 0
#define ALPHA_LTA 0.9
#define BETA      0.1

#define CUTPURENOISE 0//1
#define NOISE_FRM 32

#define   WAV_VAT   0 //0:WAV, 1:VAT
#define   FILE_HEADER_LEN   0 //when WAV_VAT==0

#define   DO_K_POINT 0 //0: not do ; 1: do
#if(DO_K_POINT==1)
	#define   K_POINT   2
#else
	#define   K_POINT   1 //not do k-point
#endif
