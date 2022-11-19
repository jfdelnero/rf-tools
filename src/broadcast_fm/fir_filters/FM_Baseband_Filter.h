#ifndef FM_BASEBAND_FILTER_H_
#define FM_BASEBAND_FILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 2000000 Hz

* 0 Hz - 75000 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 4.140342542562957 dB

* 78000 Hz - 1000000 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -40.07162834583716 dB

*/

#define FM_BASEBAND_FILTER_TAP_NUM 655

typedef struct {
  double history[FM_BASEBAND_FILTER_TAP_NUM];
  unsigned int last_index;
} FM_Baseband_Filter;

void FM_Baseband_Filter_init(FM_Baseband_Filter* f);
void FM_Baseband_Filter_put(FM_Baseband_Filter* f, double input);
double FM_Baseband_Filter_get(FM_Baseband_Filter* f);

#endif

