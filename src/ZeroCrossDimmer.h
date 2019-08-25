#ifndef __ZEROCROSSDIMMER_H__
#define __ZEROCROSSDIMMER_H__

typedef void (*callback_function)(void);
typedef bool (*callback_status)(void);
void ZeroCrossDimmer_init(callback_function _switchLightOff,
                          callback_function _switchLightOn,
                          callback_status _switchLightState);
void ZeroCrossDimmer_detectCross();
void ZeroCrossDimmer_enableTRIAC();
void ZeroCrossDimmer_startDimOn();
void ZeroCrossDimmer_startDimOff();
bool ZeroCrossDimmer_isDimming();
int ZeroCrossDimmer_getDimPercentage();
void ZeroCrossDimmer_setDimPercentage(int percentage);
void ZeroCrossDimmer_dim();

#endif
