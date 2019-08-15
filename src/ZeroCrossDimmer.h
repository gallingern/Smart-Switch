#ifndef __ZEROCROSSDIMMER_H__
#define __ZEROCROSSDIMMER_H__

void ZeroCrossDimmer_init();
void ZeroCrossDimmer_detectCross();
void ZeroCrossDimmer_enableTRIAC();
void ZeroCrossDimmer_startDimOn();
void ZeroCrossDimmer_startDimOff();
bool ZeroCrossDimmer_isDimming();
int ZeroCrossDimmer_getDimPercentage();
void ZeroCrossDimmer_setDimPercentage(int percentage);
void ZeroCrossDimmer_dim();

#endif
