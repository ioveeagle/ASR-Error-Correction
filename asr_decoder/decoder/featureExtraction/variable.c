#include "type_define.h"
#include "variable.h"

int DO_CMN_OR_MVN;
double HW[FRAME_SIZE];
int gb_bank[FFT_ORDER_2];
int    gb_totalFrames;
double gb_band_eng[MAX_FRM_NO][FILTERNO+1];
float  gb_eng[MAX_FRM_NO],gb_deng[MAX_FRM_NO],gb_ddeng[MAX_FRM_NO];
double gb_fft_wt[FFT_ORDER_2];

CEP_DATA *gb_cep_data,*delta_data,*delta2_data;
OUT_DATA *out_data;
/***   initial model parameters  ***/
/*
float     gb_ui[I_MODEL][I_STATE][I_MIX][INPUT], gb_vi[I_MODEL][I_STATE][I_MIX][INPUT],
          gb_di[I_MODEL][I_STATE][I_MIX],      gb_ci[I_MODEL][I_STATE][I_MIX],
          gb_ai[I_MODEL][I_STATE][I_STATE],	  gb_i_trout[I_MODEL];
*/
/***   final model parameters  ***/
/*
float     gb_uf[F_MODEL][F_STATE][F_MIX][INPUT], gb_vf[F_MODEL][F_STATE][F_MIX][INPUT],
          gb_df[F_MODEL][F_STATE][F_MIX],      gb_cf[F_MODEL][F_STATE][F_MIX],
          gb_af[F_MODEL][F_STATE][F_STATE],	  gb_f_trout[F_MODEL];
*/
/*   silence model parameters  */
/*
float     gb_us[S_MIX+1][INPUT], gb_vs[S_MIX+1][INPUT],
          gb_ds[S_MIX+1],      gb_cs[S_MIX+1];
*/
/*  global variables  */
/*
int       gb_i_mix_num[I_MODEL], gb_f_mix_num[F_MODEL];

int       gb_topnIndex[TOPN];
float    gb_topnScore[TOPN];
*/
