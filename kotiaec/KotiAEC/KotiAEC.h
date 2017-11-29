#ifndef KOTIAEC_H
#define KOTIAEC_H

#include <stdint.h>
#include "./kotilist.h"

enum AEC_CORE
{
    SPEEX_AEC,
    WEBRTC_AEC,
    WEBRTC_AECM
};

typedef struct speex_aec_property
{
    void* speex_echo_state;
    void* speex_preprocess_state;
    void* speex_preprocess_state_tmp;

    int16_t frame_size;
    int32_t filter_length;
    int32_t sample_freq;
    int32_t* nosie;
}speex_aec_property;

typedef struct webrtc_aec_property
{
    void* webrtc_aec;
    void* webrtc_ns;
    void* webrtc_agc;

    int16_t frame_size;
    int32_t sample_freq;
    int32_t sndcard_sample_freq;
    int16_t sndcard_delay_ms;
}webrtc_aec_property;

typedef struct farend_pcm_pack
{
    unsigned long time_usec;
    int flag;

    unsigned char* pcm_buf;
    unsigned int length;
}farend_pcm_pack;

#define FAREND_PCM_PACK_INITIALIZER {0, 0, 0, 0};
#define FAREND_PCM_PACK_MIN_DIFF_TIME 500000 // usec
extern int PLAYBACK_DELAY;

/**
 *      - agc_mode           : 0 - Unchanged
 *                          : 1 - Adaptive Analog Automatic Gain Control -3dBOv
 *                          : 2 - Adaptive Digital Automatic Gain Control -3dBOv
 *                          : 3 - Fixed Digital Gain 0dB
 *
 *      - ns_mode	: 0: Mild, 1: Medium , 2: Aggressive
**/
void KotiAEC_init(int16_t frame_size = 160, int32_t sample_freq = 8000, AEC_CORE aec_core = SPEEX_AEC,
                  int speex_filter_length = 160*20, int16_t agc_mode = 1, int16_t compression_gain_db = 18, uint8_t limiter_enable = 0, int ns_mode = 1, float snd_amplification = 1.0f);
int KotiAEC_process(const int16_t* farend, const int16_t* nearend, int16_t* out);
void KotiAEC_destory();
int KotiAEC_agc(int16_t* out);

int speex_aec_playback_for_async(int16_t* farend, float snd_amplification = 1.0f);
void push_back_farend_pcm_packs(const farend_pcm_pack& pcm_pack);
koti::list<farend_pcm_pack>::iterator find_optimal_of_farend_pcm_packs(unsigned long time_flag);
const farend_pcm_pack* front_of_farend_pcm_packs();
void erase_from_farend_pcm_packs();
unsigned int farend_pcm_packs_length();

void set_sndcard_delay_ms_for_webrtc(int16_t ms);

#endif // KOTIAEC_H
