#ifndef FIR_AUDIO_FILTER_FILTER_H_
#define FIR_AUDIO_FILTER_FILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 200000 Hz

* 0 Hz - 15000 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 4.131020865324735 dB

* 16600 Hz - 100000 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -40.090485410010906 dB

*/

#define FIR_AUDIO_FILTER_FILTER_TAP_NUM 125

typedef struct {
  double history[FIR_AUDIO_FILTER_FILTER_TAP_NUM];
  unsigned int last_index;
} FIR_Audio_Filter_Filter;

void FIR_Audio_Filter_Filter_init(FIR_Audio_Filter_Filter* f);
void FIR_Audio_Filter_Filter_put(FIR_Audio_Filter_Filter* f, double input);
double FIR_Audio_Filter_Filter_get(FIR_Audio_Filter_Filter* f);

#endif
