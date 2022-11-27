#ifndef FIR_RDS_PASSBAND_FILTER_H_
#define FIR_RDS_PASSBAND_FILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 200000 Hz

* 0 Hz - 53500 Hz
  gain = 0
  desired attenuation = -45 dB
  actual attenuation = -47.361757711285 dB

* 55000 Hz - 59000 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 3.1644314706123344 dB

* 60500 Hz - 100000 Hz
  gain = 0
  desired attenuation = -45 dB
  actual attenuation = -47.320695886416395 dB

*/

#define FIR_RDS_PASSBAND_FILTER_TAP_NUM 181

typedef struct {
  double history[FIR_RDS_PASSBAND_FILTER_TAP_NUM];
  unsigned int last_index;
} FIR_RDS_Passband_Filter;

void FIR_RDS_Passband_Filter_init(FIR_RDS_Passband_Filter* f);
void FIR_RDS_Passband_Filter_put(FIR_RDS_Passband_Filter* f, double input);
double FIR_RDS_Passband_Filter_get(FIR_RDS_Passband_Filter* f);

#endif
