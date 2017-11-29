#include "./KotiAEC.h"

#include <stddef.h>
#include <malloc.h>

#define WEBRTC_AEC_CORE_ENABLED

#ifdef OLD_SPEEX_AEC
#include "./SpeexAEC/speex_echo.h"
#include "./SpeexAEC/speex_preprocess.h"
#else
#include "./SpeexAEC1.2/speex_echo.h"
#include "./SpeexAEC1.2/speex_preprocess.h"
#endif
#define FILTER_LENGTH_MULTIPLE_OF_FRAME_SIZE 20

static koti::list<farend_pcm_pack> farend_pcm_packs;
#define FAREND_PCM_PACKS_MAX_LENGTH 100

#ifdef WEBRTC_AEC_CORE_ENABLED
#include "./WebrtcAEC/include/echo_cancellation.h"
#include "./WebrtcAEC/include/echo_control_mobile.h"
#include "./WebrtcAEC/include/gain_control.h"
#include "./WebrtcAEC/include/noise_suppression.h"
#include "./WebrtcAEC/include/noise_suppression_x.h"
#endif
#include <stdio.h>

int PLAYBACK_DELAY = 4;
static speex_aec_property speex_aec_pty = {NULL, NULL, NULL, 0, 0, 0, NULL};
static webrtc_aec_property webrtc_aec_pty = {NULL, NULL, NULL, 0, 0, 0, 0};
static webrtc_aec_property webrtc_aecm_pty = {NULL, NULL, NULL, 0, 0, 0, 0};
static float output_sound_amplification = 1.0f;
static AEC_CORE aec_core_used = SPEEX_AEC;

void KotiAEC_init(int16_t frame_size, int32_t sample_freq, AEC_CORE aec_core,
                  int speex_filter_length, int16_t agc_mode, int16_t compression_gain_db, uint8_t limiter_enable, int ns_mode, float snd_amplification)
{
    int32_t speex_agc = 0; float speex_agc_level = 24000, speex_agc_level_tmp = 3000;
    int32_t speex_noise_suppress = -15;
    aec_core_used = aec_core;
    output_sound_amplification = snd_amplification;

    switch(aec_core)
    {
#ifdef WEBRTC_AEC_CORE_ENABLED
    case WEBRTC_AEC:
        if(webrtc_aec_pty.webrtc_aec == NULL)
        {
            webrtc_aec_pty.frame_size = frame_size;
            webrtc_aec_pty.sample_freq = sample_freq;
            webrtc_aec_pty.sndcard_sample_freq = sample_freq;
            webrtc_aec_pty.sndcard_delay_ms = ((float)speex_filter_length/sample_freq) * 1000;
            if( WebRtcAec_Create(&webrtc_aec_pty.webrtc_aec) == 0)
                WebRtcAec_Init(webrtc_aec_pty.webrtc_aec, webrtc_aec_pty.sample_freq, webrtc_aec_pty.sndcard_sample_freq);
            if( WebRtcNsx_Create((NsxHandle**)&webrtc_aec_pty.webrtc_ns) == 0)
            {
                WebRtcNsx_Init((NsxHandle*)webrtc_aec_pty.webrtc_ns, webrtc_aec_pty.sample_freq);
                WebRtcNsx_set_policy((NsxHandle*)webrtc_aec_pty.webrtc_ns, ns_mode);
            }
            if( WebRtcAgc_Create(&webrtc_aec_pty.webrtc_agc) == 0)
            {
                WebRtcAgc_Init(webrtc_aec_pty.webrtc_agc, 0, 255, agc_mode/*kAgcModeFixedDigital*/, webrtc_aec_pty.sample_freq);
                WebRtcAgc_config_t conf = {0, compression_gain_db, limiter_enable};
                WebRtcAgc_set_config(webrtc_aec_pty.webrtc_agc, conf);
            }
        }
        else
        {
            WebRtcAec_Init(webrtc_aec_pty.webrtc_aec, webrtc_aec_pty.sample_freq, webrtc_aec_pty.sndcard_sample_freq);
        }
        break;
    case WEBRTC_AECM:
        if(webrtc_aecm_pty.webrtc_aec == NULL)
        {
            webrtc_aecm_pty.frame_size = frame_size;
            webrtc_aecm_pty.sample_freq = sample_freq;
            webrtc_aecm_pty.sndcard_sample_freq = sample_freq;
            webrtc_aecm_pty.sndcard_delay_ms = ((float)speex_filter_length/sample_freq) * 1000;
//            webrtc_aecm_pty.sndcard_delay_ms = speex_filter_length/frame_size;
            if( WebRtcAecm_Create(&webrtc_aecm_pty.webrtc_aec) == 0)
                WebRtcAecm_Init(webrtc_aecm_pty.webrtc_aec, webrtc_aecm_pty.sample_freq);
            if( WebRtcNsx_Create((NsxHandle**)&webrtc_aecm_pty.webrtc_ns) == 0)
            {
                WebRtcNsx_Init((NsxHandle*)webrtc_aecm_pty.webrtc_ns, webrtc_aecm_pty.sample_freq);
                WebRtcNsx_set_policy((NsxHandle*)webrtc_aecm_pty.webrtc_ns, ns_mode);
            }
            if( WebRtcAgc_Create(&webrtc_aecm_pty.webrtc_agc) == 0)
            {
                WebRtcAgc_Init(webrtc_aecm_pty.webrtc_agc, 0, 255, agc_mode/*kAgcModeFixedDigital*/, webrtc_aecm_pty.sample_freq);
                WebRtcAgc_config_t conf = {0, compression_gain_db, limiter_enable};
                WebRtcAgc_set_config(webrtc_aecm_pty.webrtc_agc, conf);
            }
        }
        break;
#endif
    case SPEEX_AEC:
    default:
#ifdef OLD_SPEEX_AEC
        if(speex_aec_pty.speex_echo_state == NULL && speex_aec_pty.speex_preprocess_state == NULL)
        {
            speex_aec_pty.frame_size = frame_size;
            speex_aec_pty.sample_freq = sample_freq;
            speex_aec_pty.filter_length = speex_filter_length;
            if(speex_aec_pty.filter_length < frame_size)
                speex_aec_pty.filter_length = frame_size*FILTER_LENGTH_MULTIPLE_OF_FRAME_SIZE;

            speex_aec_pty.speex_echo_state = speex_echo_state_init(frame_size, speex_aec_pty.filter_length);
            speex_aec_pty.speex_preprocess_state = speex_preprocess_state_init(frame_size, sample_freq);
            speex_echo_ctl((SpeexEchoState*)speex_aec_pty.speex_echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &speex_aec_pty.sample_freq);

            speex_aec_pty.nosie = (int32_t*) malloc( (frame_size+1) * sizeof(int32_t) );
        }
#else
        if(speex_aec_pty.speex_echo_state == NULL && speex_aec_pty.speex_preprocess_state == NULL)
        {
            speex_aec_pty.frame_size = frame_size;
            speex_aec_pty.sample_freq = sample_freq;
            speex_aec_pty.filter_length = speex_filter_length;
            if(speex_aec_pty.filter_length < frame_size)
                speex_aec_pty.filter_length = frame_size*FILTER_LENGTH_MULTIPLE_OF_FRAME_SIZE;

            speex_aec_pty.speex_echo_state = speex_echo_state_init(frame_size, speex_aec_pty.filter_length);
            speex_aec_pty.speex_preprocess_state = speex_preprocess_state_init(frame_size, sample_freq);
            speex_echo_ctl((SpeexEchoState*)speex_aec_pty.speex_echo_state, SPEEX_ECHO_SET_SAMPLING_RATE, &speex_aec_pty.sample_freq);
            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_SET_ECHO_STATE,
                                 speex_aec_pty.speex_echo_state);

            speex_aec_pty.nosie = (int32_t*) malloc( (frame_size+1) * sizeof(int32_t) );

            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_SET_AGC, &speex_agc);
            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_SET_AGC_LEVEL,
                                 &speex_agc_level);
            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,
                                 &speex_noise_suppress);

            speex_aec_pty.speex_preprocess_state_tmp = speex_preprocess_state_init(frame_size, sample_freq);
            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state_tmp, SPEEX_PREPROCESS_SET_AGC, &speex_agc);
            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state_tmp, SPEEX_PREPROCESS_SET_AGC_LEVEL,
                                 &speex_agc_level_tmp);
            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state_tmp, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,
                                 &speex_noise_suppress);

//            int32_t speex_vad = 1;
//            int32_t speex_vad_prob_start = 80;
//            int32_t speex_vad_prob_continue = 65;
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_SET_VAD, &speex_vad);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_SET_PROB_START,
//                                 &speex_vad_prob_start);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_SET_PROB_CONTINUE,
//                                 &speex_vad_prob_continue);

//            int32_t tmp = -1; float tmp1 = 0.0f;
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_DENOISE, &tmp);
//            printf("SPEEX_PREPROCESS_GET_DENOISE: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_NOISE_SUPPRESS, &tmp);
//            printf("SPEEX_PREPROCESS_GET_NOISE_SUPPRESS: %d\n", tmp);

//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_AGC, &tmp);
//            printf("SPEEX_PREPROCESS_GET_AGC: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_VAD, &tmp);
//            printf("SPEEX_PREPROCESS_GET_VAD: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_PROB_START, &tmp);
//            printf("SPEEX_PREPROCESS_GET_PROB_START: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_PROB_CONTINUE, &tmp);
//            printf("SPEEX_PREPROCESS_GET_PROB_CONTINUE: %d\n", tmp);

//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_AGC_LEVEL, &tmp1);
//            printf("SPEEX_PREPROCESS_GET_AGC_LEVEL: %f\n", tmp1);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_AGC_DECREMENT, &tmp);
//            printf("SPEEX_PREPROCESS_GET_AGC_DECREMENT: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_AGC_INCREMENT, &tmp);
//            printf("SPEEX_PREPROCESS_GET_AGC_INCREMENT: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_AGC_MAX_GAIN, &tmp);
//            printf("SPEEX_PREPROCESS_GET_AGC_MAX_GAIN: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_AGC_GAIN, &tmp);
//            printf("SPEEX_PREPROCESS_GET_AGC_GAIN: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_AGC_TARGET, &tmp);
//            printf("SPEEX_PREPROCESS_GET_AGC_TARGET: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_AGC_LOUDNESS, &tmp);
//            printf("SPEEX_PREPROCESS_GET_AGC_LOUDNESS: %d\n", tmp);

//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_ECHO_SUPPRESS, &tmp);
//            printf("SPEEX_PREPROCESS_GET_ECHO_SUPPRESS: %d\n", tmp);
//            speex_preprocess_ctl((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, SPEEX_PREPROCESS_GET_ECHO_SUPPRESS_ACTIVE, &tmp);
//            printf("SPEEX_PREPROCESS_GET_ECHO_SUPPRESS_ACTIVE: %d\n", tmp);
        }
#endif

        break;
    }
}

static int16_t proc_tmp_buf[320] = {0};
int KotiAEC_process(const int16_t* farend, const int16_t* nearend, int16_t* out)
{
    int ret = -1, i = 0, frame_size = 0;
    switch(aec_core_used)
    {
#ifdef WEBRTC_AEC_CORE_ENABLED
    case WEBRTC_AEC:
        if(farend)
            WebRtcAec_BufferFarend(webrtc_aec_pty.webrtc_aec, farend, webrtc_aec_pty.frame_size);
        if(!WebRtcAec_Process(webrtc_aec_pty.webrtc_aec, nearend, NULL, out, NULL, webrtc_aec_pty.frame_size,
                                   webrtc_aec_pty.sndcard_delay_ms, 0))
        {
            ret = 0;
        }

        if(webrtc_aec_pty.webrtc_ns)
        {
            WebRtcNsx_Process((NsxHandle*)webrtc_aec_pty.webrtc_ns, out, NULL, out, NULL);
            if(webrtc_aec_pty.frame_size == 160)
                WebRtcNsx_Process((NsxHandle*)webrtc_aec_pty.webrtc_ns, out+80, NULL, out+80, NULL);
        }

        if(webrtc_aec_pty.webrtc_agc)
        {
            int32_t out_c; uint8_t warn_status;
            WebRtcAgc_Process(webrtc_aec_pty.webrtc_agc, out, NULL, webrtc_aec_pty.frame_size, out, NULL, 32,
                              &out_c, 1, &warn_status);
        }

//        if(webrtc_aec_pty.webrtc_ns)
//        {
//            WebRtcNsx_Process((NsxHandle*)webrtc_aec_pty.webrtc_ns, out, NULL, out, NULL);
//            if(webrtc_aec_pty.frame_size == 160)
//                WebRtcNsx_Process((NsxHandle*)webrtc_aec_pty.webrtc_ns, out+80, NULL, out+80, NULL);
//        }

        frame_size = webrtc_aec_pty.frame_size;
        break;
    case WEBRTC_AECM:
/*
        if(farend)
            WebRtcAecm_BufferFarend(webrtc_aecm_pty.webrtc_aec, farend, webrtc_aecm_pty.frame_size);
        if(!WebRtcAecm_Process(webrtc_aecm_pty.webrtc_aec, nearend, NULL, out, webrtc_aecm_pty.frame_size,
                              webrtc_aecm_pty.sndcard_delay_ms))
        {
            ret = 0;
        }
*/

	memcpy(proc_tmp_buf, nearend, webrtc_aecm_pty.frame_size*2);
        if(webrtc_aecm_pty.webrtc_ns)
        {
            WebRtcNsx_Process((NsxHandle*)webrtc_aecm_pty.webrtc_ns, proc_tmp_buf, NULL, proc_tmp_buf, NULL);
            if(webrtc_aecm_pty.frame_size == 160)
                WebRtcNsx_Process((NsxHandle*)webrtc_aecm_pty.webrtc_ns, proc_tmp_buf+80, NULL, proc_tmp_buf+80, NULL);
        }

        if(webrtc_aecm_pty.webrtc_agc)
        {
            int32_t out_c; uint8_t warn_status;
            WebRtcAgc_Process(webrtc_aecm_pty.webrtc_agc, proc_tmp_buf, NULL, webrtc_aecm_pty.frame_size, proc_tmp_buf, NULL, 32,
                              &out_c, 1, &warn_status);
        }

	// AEC
        if(farend)
            WebRtcAecm_BufferFarend(webrtc_aecm_pty.webrtc_aec, farend, webrtc_aecm_pty.frame_size);
        if(!WebRtcAecm_Process(webrtc_aecm_pty.webrtc_aec, proc_tmp_buf, NULL, out, webrtc_aecm_pty.frame_size,
                              webrtc_aecm_pty.sndcard_delay_ms))
        {
            ret = 0;
        }

        frame_size = webrtc_aecm_pty.frame_size;
        break;
#endif
    case SPEEX_AEC:
    default:
#ifdef OLD_SPEEX_AEC
        speex_echo_cancel((SpeexEchoState*)speex_aec_pty.speex_echo_state, nearend, farend, out, speex_aec_pty.nosie);
        if(speex_preprocess((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, out, speex_aec_pty.nosie) == 1)
            ret = 0;
#else
        if(farend)
            speex_echo_cancellation((SpeexEchoState*)speex_aec_pty.speex_echo_state, nearend, farend, out);
        else
            speex_echo_capture((SpeexEchoState*)speex_aec_pty.speex_echo_state, nearend, out);
//        speex_preprocess_estimate_update((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, out);
        if(speex_preprocess_run((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state, out) == 1)
            ret = 0;
#endif

        frame_size = speex_aec_pty.frame_size;
        break;
    }

    // if the output sound needed amplify
    if(output_sound_amplification != 1.0f && output_sound_amplification > 0)
    {
        for(; i<frame_size; ++i)
            out[i] = out[i]*output_sound_amplification;
    }

    return ret;
}

int KotiAEC_agc(int16_t* out)
{
    int ret = -1;
    switch(aec_core_used)
    {
#ifdef WEBRTC_AEC_CORE_ENABLED
    case WEBRTC_AEC:
        if(webrtc_aec_pty.webrtc_ns)
        {
            WebRtcNsx_Process((NsxHandle*)webrtc_aec_pty.webrtc_ns, out, NULL, out, NULL);
            if(webrtc_aec_pty.frame_size == 160)
                WebRtcNsx_Process((NsxHandle*)webrtc_aec_pty.webrtc_ns, out+80, NULL, out+80, NULL);
        }

        if(webrtc_aec_pty.webrtc_agc)
        {
            int32_t out_c; uint8_t warn_status;
            WebRtcAgc_Process(webrtc_aec_pty.webrtc_agc, out, NULL, webrtc_aec_pty.frame_size, out, NULL, 32,
                              &out_c, 1, &warn_status);
        }
        ret = 0;

        break;
    case WEBRTC_AECM:
        if(webrtc_aecm_pty.webrtc_ns)
        {
            WebRtcNsx_Process((NsxHandle*)webrtc_aecm_pty.webrtc_ns, out, NULL, out, NULL);
            if(webrtc_aecm_pty.frame_size == 160)
                WebRtcNsx_Process((NsxHandle*)webrtc_aecm_pty.webrtc_ns, out+80, NULL, out+80, NULL);
        }

        if(webrtc_aecm_pty.webrtc_agc)
        {
            int32_t out_c; uint8_t warn_status;
            WebRtcAgc_Process(webrtc_aecm_pty.webrtc_agc, out, NULL, webrtc_aecm_pty.frame_size, out, NULL, 32,
                              &out_c, 1, &warn_status);
        }
        ret = 0;

        break;
#endif
    case SPEEX_AEC:
    default:
        break;
    }

    return ret;
}

void KotiAEC_destory()
{
    if(speex_aec_pty.speex_echo_state != NULL && speex_aec_pty.speex_preprocess_state != NULL)
    {
        speex_echo_state_destroy((SpeexEchoState*)speex_aec_pty.speex_echo_state);
        speex_preprocess_state_destroy((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state);

        if(speex_aec_pty.speex_preprocess_state_tmp)
            speex_preprocess_state_destroy((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state_tmp);

        if(speex_aec_pty.nosie)
            free(speex_aec_pty.nosie);

        speex_aec_pty.speex_echo_state = NULL;
        speex_aec_pty.speex_preprocess_state = NULL;
        speex_aec_pty.speex_preprocess_state_tmp = NULL;
        speex_aec_pty.frame_size = 0;
        speex_aec_pty.sample_freq = 0;
        speex_aec_pty.filter_length = 0;
        speex_aec_pty.nosie = NULL;
    }

#ifdef WEBRTC_AEC_CORE_ENABLED
    if(webrtc_aec_pty.webrtc_aec != NULL)
    {
        WebRtcAec_Free(webrtc_aec_pty.webrtc_aec);
        webrtc_aec_pty.webrtc_aec = NULL;
        webrtc_aec_pty.frame_size = 0;
        webrtc_aec_pty.sample_freq = 0;
        webrtc_aec_pty.sndcard_sample_freq = 0;

        if(webrtc_aec_pty.webrtc_ns)
        {
            WebRtcNsx_Free((NsxHandle*)webrtc_aec_pty.webrtc_ns);
            webrtc_aec_pty.webrtc_ns = NULL;
        }
        if(webrtc_aec_pty.webrtc_agc)
        {
            WebRtcAgc_Free(webrtc_aec_pty.webrtc_agc);
            webrtc_aec_pty.webrtc_agc = NULL;
        }
    }

    if(webrtc_aecm_pty.webrtc_aec != NULL)
    {
        WebRtcAecm_Free(webrtc_aecm_pty.webrtc_aec);
        webrtc_aecm_pty.webrtc_aec = NULL;
        webrtc_aecm_pty.frame_size = 0;
        webrtc_aecm_pty.sample_freq = 0;
        webrtc_aecm_pty.sndcard_sample_freq = 0;

        if(webrtc_aecm_pty.webrtc_ns)
        {
            WebRtcNsx_Free((NsxHandle*)webrtc_aecm_pty.webrtc_ns);
            webrtc_aecm_pty.webrtc_ns = NULL;
        }
        if(webrtc_aecm_pty.webrtc_agc)
        {
            WebRtcAgc_Free(webrtc_aecm_pty.webrtc_agc);
            webrtc_aecm_pty.webrtc_agc = NULL;
        }
    }
#endif
}

int speex_aec_playback_for_async(int16_t* farend, float snd_amplification)
{
    int ret_val = -1;
    switch(aec_core_used)
    {
#ifdef WEBRTC_AEC_CORE_ENABLED
    case WEBRTC_AEC:
        if(webrtc_aec_pty.webrtc_aec)
            ret_val = WebRtcAec_BufferFarend(webrtc_aec_pty.webrtc_aec, farend, webrtc_aec_pty.frame_size);
        break;
    case WEBRTC_AECM:
        if(webrtc_aecm_pty.webrtc_aec)
            ret_val = WebRtcAecm_BufferFarend(webrtc_aecm_pty.webrtc_aec, farend, webrtc_aecm_pty.frame_size);
        break;
#endif
    case SPEEX_AEC:
    default:
        if(speex_aec_pty.speex_echo_state)
        {
            speex_echo_playback((SpeexEchoState*)speex_aec_pty.speex_echo_state, farend);
    //        speex_preprocess_run((SpeexPreprocessState*)speex_aec_pty.speex_preprocess_state_tmp, farend);
            ret_val = 0;
        }
        break;
    }

    if(snd_amplification != 1.0f && snd_amplification > 0)
    {
        for(int i=0; i<speex_aec_pty.frame_size; ++i)
            farend[i] = farend[i]*snd_amplification;
    }

    return ret_val;
}

void set_sndcard_delay_ms_for_webrtc(int16_t ms)
{
    webrtc_aec_pty.sndcard_delay_ms = ms;
    webrtc_aecm_pty.sndcard_delay_ms = ms;
}

void push_back_farend_pcm_packs(const farend_pcm_pack& pcm_pack)
{
    while(farend_pcm_packs.size() >= FAREND_PCM_PACKS_MAX_LENGTH)
        erase_from_farend_pcm_packs();

    farend_pcm_packs.push_back(pcm_pack);
}

koti::list<farend_pcm_pack>::iterator find_optimal_of_farend_pcm_packs(unsigned long time_flag)
{
    koti::list<farend_pcm_pack>::iterator optimal_pack_itr = NULL;
    koti::list<farend_pcm_pack>::iterator itr = farend_pcm_packs.list_begin();

    unsigned long farend_pcm_pack_time = 0;
    int diff_time = FAREND_PCM_PACK_MIN_DIFF_TIME;
    for(; itr != farend_pcm_packs.list_end(); itr=itr->next)
    {
        farend_pcm_pack_time = (itr->pointer)->time_usec;
        int tmp_diff_time = time_flag - farend_pcm_pack_time;
        if(tmp_diff_time < 0)
            break;

        if(tmp_diff_time < diff_time)
        {
            optimal_pack_itr = itr;
            diff_time = tmp_diff_time;
        }
    }

    return optimal_pack_itr;
}

const farend_pcm_pack* front_of_farend_pcm_packs()
{
    return farend_pcm_packs.front();
}

void erase_from_farend_pcm_packs()
{
    if(farend_pcm_packs.size() > 0)
    {
        if(farend_pcm_packs.front()->pcm_buf)
            free(farend_pcm_packs.front()->pcm_buf);

        farend_pcm_packs.erase_front();
    }
}

unsigned int farend_pcm_packs_length()
{
    return farend_pcm_packs.size();
}
