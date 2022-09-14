#include "type_define.h"

#ifdef _DEBUG
#include <time.h>
#endif

#ifdef SSE2
#include <emmintrin.h>
#define ALIGNSIZE	16
#define ALIGNTYPE	__attribute__(aligned(ALIGNSIZE))	// for GCC
#endif

#ifdef AVX 
#include <immintrin.h>
#define ALIGNSIZE	32
#define ALIGNTYPE	__attribute__(aligned(ALIGNSIZE))	// for GCC
#endif


class FEATURE{
public:
int DO_CMN_OR_MVN;
// the following is from 'variable.c'
OUT_DATA *out_data;

void doInitial();
void doExtract(char*, char*, int);
void doReset();
void checkArgs(int, char**);

private:
int MakeDirectory(char *filename);
int filesize(FILE*);
void CEPLIFTER(int);
void SubDC(float*);
void getDeltaCep_New(int, float (*)[13], float (*)[13]);
void MVN(float (*)[39], int);
void CMN_New(float (*)[13], int);
int OutputFeature(char*);
void byteSwapShort(short int*);
void byteSwapInt(int*);
void byteSwapArray(float*, int);
int OutputFeatureHTKFormat(char*, char*);
int AssignCepToOutData();

float Mel(int k);
void SetFilterBanks(double w[]);
void PreEmphasis(float s[]);
void GetHammingWindow();
void Haming_window(float s[]);
void fft(int inv,int n,COMPLEX *z_data);
int MelBandMagnitude(short *sample);
void DCT(CEP_DATA cep[]);
void EnergyContour(float* eng,double *pwr);
void LTA222();
int getTotalFrame(int sampleNumber);
void ini_fea_para();
void getDeltaCep(int totalFrames,CEP_DATA *cep,CEP_DATA *deltaCep);
void getDDEng(int totalFrames,float *eng,float *dEng,float *ddEng);
//int MFCC_shell(char *fname,FEA_DATA fea[]);
int MFCC_shell(char *fname);
void CMN(CEP_DATA *Cep_Data,int begin,int end,int frameNumber);

// the followings are from 'variable.c'
#ifdef SSE2
float HW[FRAME_SIZE];
#else
double HW[FRAME_SIZE];
#endif
int gb_bank[FFT_ORDER_2];
int    gb_totalFrames;
double gb_band_eng[MAX_FRM_NO][FILTERNO+1];
float  gb_eng[MAX_FRM_NO],gb_deng[MAX_FRM_NO],gb_ddeng[MAX_FRM_NO];
double gb_fft_wt[FFT_ORDER_2];
CEP_DATA *gb_cep_data,*delta_data,*delta2_data;

// the followings are from 'tcos.c'
static const float tcos[CEP_DIM+1][FILTERNO+1];

char extensionName[64];

};


