#include <stdint.h>

void radio_init();
void radio_jam(uint16_t center_freq_mhz, uint32_t duration_ms);
void radio_squarewave_fm_init(uint16_t carrier_mhz, uint8_t atten);
void radio_squarewave_fm(uint16_t carrier_mhz, uint16_t signal_hz, uint32_t duration_ms);
void radio_sin_am(uint16_t carrier_mhz, uint16_t signal_hz, uint32_t duration_ms);
