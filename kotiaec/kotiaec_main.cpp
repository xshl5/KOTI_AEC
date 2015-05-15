// echo.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#if 1
#define ref_filename "/kotidata/aec_ref.pcm"
#define mic_filename "/kotidata/aec_mic.pcm"
#define echo_filename "/kotidata/out.pcm"
#else
#define ref_filename "farend.pcm"
#define mic_filename "nearend.pcm"
#define echo_filename "out.pcm"
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "./KotiAEC.h"
#include "./kotilist.h"
#include <stdlib.h>

#define NN 160
#define TAIL NN*10

int main(void)
{
    //int echo_fd, ref_fd, e_fd;
    FILE* ref_fd, *mic_fd, *echo_fd;
    short mic_buf[NN+1], ref_buf[NN+1],out_buf[NN+1];

    ref_fd = fopen (ref_filename, "rb");///打开参考文件，即要消除的声音
    mic_fd = fopen (mic_filename,  "rb");//打开mic采集到的声音文件，包含回声在里面
    echo_fd = fopen (echo_filename, "wb");//消除了回声以后的文件

    if(ref_fd == NULL || mic_fd == NULL || echo_fd == NULL)
    {
        printf("File opened error.\n");
        return -1;
    }

    FILE* fp;
    char tmp[16] = {0};
    int multiple = 19, aec_core_selected = 2;
    if( (fp = fopen("/kotidata/aec_conf", "r")) )
    {
        if( fgets(tmp, 16, fp) )
        {
            multiple = atoi(tmp);

            if(fgets(tmp, 16, fp))
                aec_core_selected = atoi(tmp);
        }
        fclose(fp);
    }
    KotiAEC_init(NN, 8000, (AEC_CORE)aec_core_selected, NN*multiple);
    printf("====================================> %d, %d\n", aec_core_selected, multiple);

    while (fread(mic_buf,1, NN*2,mic_fd))
    {
        fread(ref_buf, 1, NN*2,ref_fd);
        speex_aec_playback_for_async(ref_buf);
        int xxx = KotiAEC_process(NULL, mic_buf, out_buf);
//        printf("=================================== %d\n", xxx);
        fwrite(out_buf,1, NN*2,echo_fd);
    }

//    for(int i=0; i<10; ++i)
//    {
//        farend_pcm_pack pcm_pack = FAREND_PCM_PACK_INITIALIZER;
//        struct timeval tv = {0, 0};
//        gettimeofday(&tv, NULL);
//        pcm_pack.time_usec = tv.tv_sec*1000000 + tv.tv_usec;
//        pcm_pack.length = 160;
//        pcm_pack.pcm_buf = (unsigned char*)malloc(160);
//        push_back_farend_pcm_packs(pcm_pack);
//    }

//    sleep(1);
//    struct timeval tv = {0, 0};
//    gettimeofday(&tv, NULL);
//    unsigned long time_usec = tv.tv_sec*1000000 + tv.tv_usec;
//    koti::list<farend_pcm_pack>::iterator itr = find_optimal_of_farend_pcm_packs(time_usec);

    KotiAEC_destory();
    fclose(mic_fd);
    fclose(echo_fd);
    fclose(ref_fd);
    return 0;
}


