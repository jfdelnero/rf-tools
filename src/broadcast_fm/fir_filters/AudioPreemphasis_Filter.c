#include "AudioPreemphasis_Filter.h"

static double filter_taps[AUDIOPREEMPHASIS_FILTER_TAP_NUM] = {
  -2.676917973553303,
  6.337642794858812,
  -2.676917973553303
};

void AudioPreemphasis_Filter_init(AudioPreemphasis_Filter* f) {
  int i;
  for(i = 0; i < AUDIOPREEMPHASIS_FILTER_TAP_NUM; ++i)
    f->history[i] = 0;
  f->last_index = 0;
}

void AudioPreemphasis_Filter_put(AudioPreemphasis_Filter* f, double input) {
  f->history[f->last_index++] = input;
  if(f->last_index == AUDIOPREEMPHASIS_FILTER_TAP_NUM)
    f->last_index = 0;
}

double AudioPreemphasis_Filter_get(AudioPreemphasis_Filter* f) {
  double acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < AUDIOPREEMPHASIS_FILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : AUDIOPREEMPHASIS_FILTER_TAP_NUM-1;
    acc += f->history[index] * filter_taps[i];
  };
  return acc;
}
