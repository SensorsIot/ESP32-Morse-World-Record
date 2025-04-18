#include "radios.h"
#include "radios_lut.h"
#include "esp_timer.h"
#include <rom/ets_sys.h>


extern void wifiscwout(uint32_t *param);
extern void rftest_init();
extern void phy_set_freq(unsigned int freq_mhz, int freq_khz);
extern void start_tx_tone(uint8_t p1_c_1,int sometimes_0,uint8_t att,uint8_t p4_c_0,int p5_c_0, uint8_t p6_c_0);
extern void force_txon(uint8_t onoff);


void radio_init()
{
	rftest_init();
}

void radio_jam(uint16_t center_freq_mhz, uint32_t duration_ms)
{
    uint32_t params[] = {
        1, // enable
        6, // channel
        0  // atten in x0.25dB?
    };
    wifiscwout(params); // start with full power
    // alternatively, start radio with this:
    // bt_zb_tx_tone(1, 1, 1, 1);
    // or this:
    // force_txon(1);
    int repeat = (duration_ms * 1000) / (266 * sizeof(sin_lut_A1000_256));

    int16_t offset = 500;
    for (int k = 0; k < repeat; k++)
    {
        for (int i = 0; i < sizeof(sin_lut_A1000_256); i++)
        {
            phy_set_freq(center_freq_mhz, offset + sin_lut_A1000_256[i]);
        }
    }
}

void radio_squarewave_fm_init(uint16_t carrier_mhz, uint8_t atten)
{
    force_txon(1);
    start_tx_tone(1, 0, atten, 0, 0, 0);
    phy_set_freq(carrier_mhz, 0);
}

void radio_squarewave_fm(uint16_t carrier_mhz, uint16_t signal_hz, uint32_t duration_ms)
{
    int64_t delay_us = (1000000 - 266) / signal_hz;
    if(delay_us < 0) delay_us = 0;
    uint32_t repeat = ((duration_ms * 1000) / (delay_us + 266)) >> 1;
    // printf("carrier_mhz: %i, signal_hz: %i, duration_ms: %li\ndelay_us: %lli, repeat: %li\n", carrier_mhz, signal_hz, duration_ms, delay_us, repeat);

    for(int i = 0; i < repeat; i++) {
        phy_set_freq(carrier_mhz, 100);
        ets_delay_us(delay_us);
        phy_set_freq(carrier_mhz, 0);
        ets_delay_us(delay_us);
    }
}

void radio_sin_am(uint16_t carrier_mhz, uint16_t signal_hz, uint32_t duration_ms)
{
    int64_t delay_us = (1000000 - 2) / (signal_hz * sizeof(sin_lut64_32));
    if(delay_us < 0) delay_us = 0;
    uint32_t repeat = (((duration_ms * 1000) - 266) / ((delay_us + 2) * sizeof(sin_lut64_32)));
    // printf("carrier_mhz: %i, signal_hz: %i, duration_ms: %li\ndelay_us: %lli, repeat: %li\n", carrier_mhz, signal_hz, duration_ms, delay_us, repeat);

    phy_set_freq(carrier_mhz, 0);
    for(int i = 0; i < repeat; i++) {
        for(int j = 0; j < sizeof(sin_lut64_32); j++) {
            start_tx_tone(1, 0, 127 - 27 - sin_lut64_32[j], 0, 0, 0);
            ets_delay_us(delay_us);
        }
    }
}

