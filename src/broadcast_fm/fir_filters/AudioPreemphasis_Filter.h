#ifndef AUDIOPREEMPHASIS_FILTER_H_
#define AUDIOPREEMPHASIS_FILTER_H_

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 50000 Hz

* 0 Hz - 2100 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = 1.2624293131559894 dB

* 15500 Hz - 25000 Hz
  gain = 6.309573444801933
  desired ripple = 5 dB
  actual ripple = 2.740064802010604 dB

*/

#define AUDIOPREEMPHASIS_FILTER_TAP_NUM 3

typedef struct {
  double history[AUDIOPREEMPHASIS_FILTER_TAP_NUM];
  unsigned int last_index;
} AudioPreemphasis_Filter;

void AudioPreemphasis_Filter_init(AudioPreemphasis_Filter* f);
void AudioPreemphasis_Filter_put(AudioPreemphasis_Filter* f, double input);
double AudioPreemphasis_Filter_get(AudioPreemphasis_Filter* f);

#endif

