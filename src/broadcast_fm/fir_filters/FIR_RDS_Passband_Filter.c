#include "FIR_RDS_Passband_Filter.h"

static double filter_taps[FIR_RDS_PASSBAND_FILTER_TAP_NUM] = {
	0.0011758952792351321,
	0.0022825939806044066,
	-0.0019570074010951343,
	-0.00027851461513982113,
	0.001982028200159741,
	-0.0003389431241327098,
	-0.0024417117605763056,
	0.0016466166949177271,
	0.00218694860144487,
	-0.0030376302189676335,
	-0.0011800497713823548,
	0.004168106876686315,
	-0.0005791258713967658,
	-0.00458555303109236,
	0.0028139691227342402,
	0.003940288276590675,
	-0.0050234537734311566,
	-0.002119154734838742,
	0.006593028363423018,
	-0.0006727491736447931,
	-0.00695399400792946,
	0.0039020880903873115,
	0.0057750793411364245,
	-0.00681158899151599,
	-0.003107033394127733,
	0.008620882212666705,
	-0.0005794133751050271,
	-0.008750162638389876,
	0.004479315885893972,
	0.0070169617420127485,
	-0.007649884221502565,
	-0.003740197001726764,
	0.009281335614837303,
	-0.0003149505855050032,
	-0.0089577356517151,
	0.0041390587641292,
	0.006802841753806556,
	-0.006771677505037472,
	-0.003472861768043537,
	0.007602360659868646,
	-0.00002158539801443246,
	-0.0065901492892861515,
	0.0026352213902105784,
	0.004304698762062282,
	-0.0036461187111267385,
	-0.0017527583320990098,
	0.0029247978193551695,
	0.000045375309167217514,
	-0.0010211980056331939,
	8.161836662908323e-7,
	-0.0009758278121958338,
	0.0017878150467851178,
	0.0017849398746189002,
	-0.00479767892252991,
	-0.000419447058223516,
	0.007723187364492702,
	-0.0034049324700813023,
	-0.008959729595498423,
	0.009016826051129821,
	0.007132775033388645,
	-0.014845503678668026,
	-0.0016410722787915503,
	0.018797163709037834,
	-0.006974139283513686,
	-0.01886428994137313,
	0.016983931436409156,
	0.013800947035325157,
	-0.025799279635182262,
	-0.0036593927759145765,
	0.03062151969130689,
	-0.00999748811830927,
	-0.029253805215330695,
	0.02431922013866165,
	0.02083281348651543,
	-0.03580207822554663,
	-0.006253984979073887,
	0.04117741905917765,
	-0.011858329290914316,
	-0.03833083360406418,
	0.02966478887325469,
	0.026988906545642496,
	-0.04301062709660084,
	-0.008948772365378355,
	0.048486879453607246,
	-0.012229670009599255,
	-0.044344388567933726,
	0.03202505232497606,
	0.031024500923222363,
	-0.04604032280096179,
	-0.01113548747503351,
	0.05109759172117753,
	-0.01113548747503351,
	-0.04604032280096179,
	0.031024500923222363,
	0.03202505232497606,
	-0.044344388567933726,
	-0.012229670009599255,
	0.048486879453607246,
	-0.008948772365378355,
	-0.04301062709660084,
	0.026988906545642496,
	0.02966478887325469,
	-0.03833083360406418,
	-0.011858329290914316,
	0.04117741905917765,
	-0.006253984979073887,
	-0.03580207822554663,
	0.02083281348651543,
	0.02431922013866165,
	-0.029253805215330695,
	-0.00999748811830927,
	0.03062151969130689,
	-0.0036593927759145765,
	-0.025799279635182262,
	0.013800947035325157,
	0.016983931436409156,
	-0.01886428994137313,
	-0.006974139283513686,
	0.018797163709037834,
	-0.0016410722787915503,
	-0.014845503678668026,
	0.007132775033388645,
	0.009016826051129821,
	-0.008959729595498423,
	-0.0034049324700813023,
	0.007723187364492702,
	-0.000419447058223516,
	-0.00479767892252991,
	0.0017849398746189002,
	0.0017878150467851178,
	-0.0009758278121958338,
	8.161836662908323e-7,
	-0.0010211980056331939,
	0.000045375309167217514,
	0.0029247978193551695,
	-0.0017527583320990098,
	-0.0036461187111267385,
	0.004304698762062282,
	0.0026352213902105784,
	-0.0065901492892861515,
	-0.00002158539801443246,
	0.007602360659868646,
	-0.003472861768043537,
	-0.006771677505037472,
	0.006802841753806556,
	0.0041390587641292,
	-0.0089577356517151,
	-0.0003149505855050032,
	0.009281335614837303,
	-0.003740197001726764,
	-0.007649884221502565,
	0.0070169617420127485,
	0.004479315885893972,
	-0.008750162638389876,
	-0.0005794133751050271,
	0.008620882212666705,
	-0.003107033394127733,
	-0.00681158899151599,
	0.0057750793411364245,
	0.0039020880903873115,
	-0.00695399400792946,
	-0.0006727491736447931,
	0.006593028363423018,
	-0.002119154734838742,
	-0.0050234537734311566,
	0.003940288276590675,
	0.0028139691227342402,
	-0.00458555303109236,
	-0.0005791258713967658,
	0.004168106876686315,
	-0.0011800497713823548,
	-0.0030376302189676335,
	0.00218694860144487,
	0.0016466166949177271,
	-0.0024417117605763056,
	-0.0003389431241327098,
	0.001982028200159741,
	-0.00027851461513982113,
	-0.0019570074010951343,
	0.0022825939806044066,
	0.0011758952792351321
};

void FIR_RDS_Passband_Filter_init(FIR_RDS_Passband_Filter* f) {
  int i;
  for(i = 0; i < FIR_RDS_PASSBAND_FILTER_TAP_NUM; ++i)
    f->history[i] = 0;
  f->last_index = 0;
}

void FIR_RDS_Passband_Filter_put(FIR_RDS_Passband_Filter* f, double input) {
  f->history[f->last_index++] = input;
  if(f->last_index == FIR_RDS_PASSBAND_FILTER_TAP_NUM)
    f->last_index = 0;
}

double FIR_RDS_Passband_Filter_get(FIR_RDS_Passband_Filter* f) {
  double acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < FIR_RDS_PASSBAND_FILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : FIR_RDS_PASSBAND_FILTER_TAP_NUM-1;
    acc += f->history[index] * filter_taps[i];
  };
  return acc;
}
