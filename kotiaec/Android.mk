LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS += -DANDROID_OS
LOCAL_MODULE_TAGS := eng
LOCAL_ARM_MODE:=arm

LOCAL_CFLAGS += -DHAVE_CONFIG_H -DWEBRTC_POSIX
LOCAL_CFLAGS += -mfloat-abi=softfp -mfpu=neon

LOCAL_SRC_FILES := \
	./KotiAEC/SpeexAEC1.2/smallft.c \
	./KotiAEC/SpeexAEC1.2/preprocess.c \
	./KotiAEC/SpeexAEC1.2/fftwrap.c \
	./KotiAEC/SpeexAEC1.2/mdf.c \
	./KotiAEC/SpeexAEC1.2/filterbank.c \
	./KotiAEC/WebrtcAEC/echo_cancellation.c \
	./KotiAEC/WebrtcAEC/aec_resampler.c \
	./KotiAEC/WebrtcAEC/aec_rdft.c \
	./KotiAEC/WebrtcAEC/aec_core.c \
	./KotiAEC/WebrtcAEC/utility/ring_buffer.c \
	./KotiAEC/WebrtcAEC/utility/randomization_functions.c \
	./KotiAEC/WebrtcAEC/utility/delay_estimator_wrapper.c \
	./KotiAEC/WebrtcAEC/utility/delay_estimator.c \
	./KotiAEC/WebrtcAEC/utility/cpu_features.cc \
	./KotiAEC/WebrtcAEC/echo_control_mobile.c \
	./KotiAEC/WebrtcAEC/aecm_core_neon_offsets.c \
	./KotiAEC/WebrtcAEC/aecm_core_neon.c \
	./KotiAEC/WebrtcAEC/aecm_core.c \
	./KotiAEC/WebrtcAEC/spl_init.c \
	./KotiAEC/WebrtcAEC/spl_sqrt_floor.c \
	./KotiAEC/WebrtcAEC/division_operations.c \
	./KotiAEC/WebrtcAEC/real_fft.c \
	./KotiAEC/WebrtcAEC/min_max_operations.c \
	./KotiAEC/WebrtcAEC/cross_correlation.c \
	./KotiAEC/WebrtcAEC/downsample_fast.c \
	./KotiAEC/WebrtcAEC/vector_scaling_operations.c \
	./KotiAEC/WebrtcAEC/complex_bit_reverse.c \
	./KotiAEC/WebrtcAEC/complex_fft.c \
	./KotiAEC/WebrtcAEC/digital_agc.c \
	./KotiAEC/WebrtcAEC/analog_agc.c \
	./KotiAEC/WebrtcAEC/resample_by_2.c \
	./KotiAEC/WebrtcAEC/spl_sqrt.c \
	./KotiAEC/WebrtcAEC/copy_set_operations.c \
	./KotiAEC/WebrtcAEC/dot_product_with_scale.c \
	./KotiAEC/WebrtcAEC/nsx_core_neon_offsets.c \
	./KotiAEC/WebrtcAEC/nsx_core_neon.c \
	./KotiAEC/WebrtcAEC/nsx_core.c \
	./KotiAEC/WebrtcAEC/ns_core.c \
	./KotiAEC/WebrtcAEC/noise_suppression_x.c \
	./KotiAEC/WebrtcAEC/noise_suppression.c \
	./KotiAEC/WebrtcAEC/energy.c \
	./KotiAEC/WebrtcAEC/fft4g.c \
	./KotiAEC/WebrtcAEC/get_scaling_square.c \
	./KotiAEC/KotiAEC.cpp 

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/KotiAEC/WebrtcAEC/include
#	$(LOCAL_PATH)/webrtc \

LOCAL_MODULE := libkoti_aec

include $(BUILD_STATIC_LIBRARY)

###################################
########## kotiaec_main ###########
###################################
include $(CLEAR_VARS)

LOCAL_CFLAGS += -DANDROID_OS
LOCAL_MODULE_TAGS := eng


LOCAL_C_INCLUDES := $(LOCAL_PATH)/KotiAEC

LOCAL_SRC_FILES := \
	kotiaec_main.cpp

LOCAL_LDFLAGS += -pie -fPIE

LOCAL_STATIC_LIBRARIES := libkoti_aec
LOCAL_MODULE := kotiaec_test

include $(BUILD_EXECUTABLE)

