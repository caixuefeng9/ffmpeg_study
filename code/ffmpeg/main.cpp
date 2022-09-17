#include <iostream>

using namespace std;

//包含ffmpeg头文件
extern "C"
{
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/audio_fifo.h"
}

#include <windows.h>
#include <vector>
#include <string>
#include <memory>

using std::vector;
using std::string;
using std::shared_ptr;

SwrContext *init_swr_ctx(void)
{
    SwrContext *swr_ctx = nullptr;
    swr_ctx = swr_alloc_set_opts(nullptr,               //ctx
                                 AV_CH_LAYOUT_STEREO,   //输出channel布局
                                 AV_SAMPLE_FMT_S16,     //输出的采样格式
                                 44100,                 //采样率
                                 AV_CH_LAYOUT_STEREO,   //输入channel布局
                                 AV_SAMPLE_FMT_S16,     //输入的采样格式
                                 44100,                 //输入的采样率
                                 0, nullptr);
    if (! swr_ctx) {
        cout<< "swr_ctx is NULL!" << endl;
        return nullptr;
    }

    if (swr_init(swr_ctx) < 0){
        cout << "swr_init is failed!" << endl;
        return nullptr;
    }

    return swr_ctx;
}

void alloc_data_4_resample(uint8_t ***src_data,
                           int *src_linesize,
                           uint8_t ***dst_data,
                           int *dst_linesize)
{
    //88200/2=44100/2=22050 如果使能对齐，则得到的src_linesize的值为22080*4=88320
    //创建输入缓冲区
    av_samples_alloc_array_and_samples(src_data,        //输入缓冲区地址
                                       src_linesize,    //缓冲区的大小
                                       2,               //通道个数
                                       22050,           //单通道采样个数
                                       AV_SAMPLE_FMT_S16,//采样格式
                                       1);              //为0，32字节对齐，为1就不对齐

    //创建输出缓冲区
    av_samples_alloc_array_and_samples(dst_data,        //输出缓冲区地址
                                       dst_linesize,    //缓冲区的大小
                                       2,               //通道个数
                                       22050,           //单通道采样个数
                                       AV_SAMPLE_FMT_S16,//采样格式
                                       1);              //为0，32字节对齐，为1就不对齐
}

AVCodecContext *open_coder(void)
{
    //打开编码器
    //avcodec_find_encoder(AV_CODEC_ID_AAC);
    const AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");

    //创建 codec 上下文
    AVCodecContext * codec_ctx = avcodec_alloc_context3(codec);

    codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    codec_ctx->channels = 2;
    codec_ctx->sample_rate = 44100;
    codec_ctx->bit_rate = 0;    //AAC_LC: 128K, AAC HE: 64K, AAC HE V2: 32K
    codec_ctx->profile = FF_PROFILE_AAC_HE_V2;  //bit_rate = 0时，profile的设置才有效

    //打开编码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        cout << "avcodec_open2 failed!" << endl;
        return nullptr;
    }

    return codec_ctx;
}

void encode(AVCodecContext *ctx,
            AVFrame *frame,
            AVPacket *pkt,
            FILE *output)
{
    int ret = 0;

    if (frame) {
        cout << "frame nb_samples:" << frame->nb_samples << "ctx frame_size:" << ctx->frame_size <<endl;
    }
    //将数据送到编码器
    ret = avcodec_send_frame(ctx, frame);

    //如果ret>=0说明数据设置成功
    while (ret >= 0) {
        //获取编码后的音频数据，如果成功（ret >= 0）需要重复获取，直到失败为止
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            cout << "Error, encoding audio frame" << endl;
            exit(-1);
        }

        //write file
        fwrite(pkt->data, static_cast<size_t>(pkt->size), 1, output);
        //刷新缓冲区，使数据写入磁盘
        fflush(output);
    }

    return;
}

/**
 * @brief open audio device
 * @param audio device name
 * @return succ: AVFormatContext*, fail: nullptr
 */
AVFormatContext *open_dev(string DeviceName)
{
    int ret = 0;
    char errors[1024];
    AVFormatContext *fmt_ctx = nullptr;
    AVDictionary *options = nullptr;
    string sDeviceName = "audio=" + DeviceName;

    //get format
    const AVInputFormat *iformat = av_find_input_format("dshow");

    //open device
    if ((ret = avformat_open_input(&fmt_ctx, sDeviceName.data(),
        iformat, &options)) < 0) {
        av_strerror(ret, errors, 1024);
        printf("Failed to open audio device, [%d]%s\n", ret, errors);
        return nullptr;
    }

    return fmt_ctx;
}

void AnsiToUtf8( const char* pchSrc, char* pchDest, int nDestLen ) 
{
	if (pchSrc == NULL || pchDest == NULL)
	{
		return;
	}

	// 先将ANSI转成Unicode
	int nUnicodeBufLen = MultiByteToWideChar(CP_ACP, 0, pchSrc, -1, NULL, 0);
	WCHAR* pUnicodeTmpBuf = new WCHAR[nUnicodeBufLen + 1];
	memset(pUnicodeTmpBuf, 0, (nUnicodeBufLen + 1) * sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, pchSrc, -1, pUnicodeTmpBuf, nUnicodeBufLen + 1);

	// 再将Unicode转成utf8
	int nUtf8BufLen = WideCharToMultiByte(CP_UTF8, 0, pUnicodeTmpBuf, -1, NULL, 0, NULL, NULL);
	char* pUtf8TmpBuf = new char[nUtf8BufLen + 1];
	memset(pUtf8TmpBuf, 0, nUtf8BufLen + 1);
	WideCharToMultiByte(CP_UTF8, 0, pUnicodeTmpBuf, -1, pUtf8TmpBuf, nUtf8BufLen + 1, NULL, NULL);

	int nLen = strlen(pUtf8TmpBuf);
	if (nLen + 1 > nDestLen)
	{
		strncpy(pchDest, pUtf8TmpBuf, nDestLen - 1);
		pchDest[nDestLen - 1] = 0;
	}
	else
	{
		strcpy(pchDest, pUtf8TmpBuf);
	}

	delete[]pUtf8TmpBuf;
	delete[]pUnicodeTmpBuf;
}

/**
 * @brief Gets the name of the audio device
 * @return A vector of audio device names
 */
vector<string> get_audio_device_name(void)
{
    // windows api 获取音频设备列表（ffmpeg好像没有提供获取音频设备的api）
    unsigned int nDeviceNum = waveInGetNumDevs();
    vector<string> vecDeviceName;

    for (unsigned int i = 0; i < nDeviceNum; i++){
        WAVEINCAPS wic;
        waveInGetDevCaps(i, &wic, sizeof (wic));
        char spDeviceName[512] = {};
        AnsiToUtf8(wic.szPname, spDeviceName, 511);
        //转成utf-8
        //int nSize = WideCharToMultiByte(CP_UTF8, 0, LPCWCH(wic.szPname),
        //            static_cast<int>(wcslen(LPCWCH(wic.szPname))), nullptr, 0, nullptr, nullptr);

        //printf("nsize:%d", nSize);
        //shared_ptr<char> spDeviceName(new char[nSize + 1]);
        //memset(spDeviceName.get(), 0, static_cast<size_t>(nSize + 1));
        //WideCharToMultiByte(CP_UTF8, 0, LPCWCH(wic.szPname), static_cast<int>(wcslen(LPCWCH(wic.szPname))),
        //                    spDeviceName.get(), nSize, nullptr, nullptr);
        //vecDeviceName.push_back(wic.szPname);
        vecDeviceName.push_back(spDeviceName);
        printf("audio input device:%s \n", spDeviceName);
    }

    return vecDeviceName;
}

/**
 * @brief creat_frame
 * @return succ: AVFrame*, fail: nullptr
 */
AVFrame *creat_frame(AVCodecContext *c_ctx)
{
    AVFrame *frame = nullptr;

    //音频输入数据(未编码的数据)
    frame = av_frame_alloc();
    if (! frame) {
        cout << "Error, Failed to alloc frame!" << endl;
        goto __ERROR;
    }

    //set parameters
    frame->nb_samples       = 2048;  //单通道一个音频帧的采样数
    frame->format           = c_ctx->sample_fmt; //AV_SAMPLE_FMT_FLTP;
    frame->channel_layout   = c_ctx->channel_layout;//AV_CH_LAYOUT_STEREO;
    frame->sample_rate = c_ctx->sample_rate;

    //alloc inner memory
    av_frame_get_buffer(frame, 1);
    if (! frame->buf[0]) {
        cout << "Error, Failed to alloc buf in frame!" << endl;
        goto __ERROR;
    }

    return frame;

__ERROR:
    if (frame) {
        av_frame_free(&frame);
    }

    return nullptr;
}

void free_data_4_resample(uint8_t **src_data, uint8_t **dst_data)
{
    //释放输入输出缓冲区
    if (src_data) {
        av_freep(&src_data[0]);
    }
    av_freep(&src_data);

    if (dst_data) {
        av_freep(&dst_data[0]);
    }
    av_freep(&dst_data);
}

int add_samples_to_fifo(AVAudioFifo *fifo,
                        uint8_t **input_data,
                        const int frame_size)
{
    int ret = 0;
    int size = 0;

    size = av_audio_fifo_size(fifo) + frame_size;
    ret = av_audio_fifo_realloc(fifo, size);
    if (ret < 0) {
        printf("Error, Failed to reallocate fifo!\n");
        return ret;
    }

    ret = av_audio_fifo_write(fifo, reinterpret_cast<void **>(input_data), frame_size);
    if (ret < frame_size) {
        printf("Error, Failed to write data to fifo!\n");
        return AVERROR_EXIT;
    }

    return 0;
}

int read_fifo_and_encode(AVAudioFifo *fifo,
                         AVFormatContext *fmt_ctx,
                         AVCodecContext *c_ctx,
                         AVFrame *frame)
{
    int ret = 0;

    const int frame_size = FFMIN(av_audio_fifo_size(fifo),
                                 c_ctx->frame_size);
    cout << "fifo size 11:" << av_audio_fifo_size(fifo) << endl;
    cout << "c_ctx->frame_size:" << c_ctx->frame_size << endl;

    ret = av_audio_fifo_read(fifo, reinterpret_cast<void **>(frame->data), frame_size);
    if (ret < frame_size) {
        printf("Error, Failed to read data from fifo!\n");
        return AVERROR_EXIT;
    }

    return 0;
}

void read_data_and_encode(AVFormatContext *fmt_ctx,
                          AVCodecContext *c_ctx,
                          SwrContext *swr_ctx,
                          FILE *outfile)
{
    int ret = 0;
    int count = 0;
    AVPacket pkt;
    AVFrame *frame = nullptr;
    AVPacket *newpkt = nullptr;
    AVAudioFifo *fifo = nullptr;

    uint8_t **src_data = nullptr;
    int src_linesize = 0;

    uint8_t **dst_data = nullptr;
    int dst_linesize = 0;

    frame = creat_frame(c_ctx);
    if (!frame) {
        printf("Error, Failed to creat frame!\n");
        goto __ERROR;
    }

    newpkt = av_packet_alloc();
    if (! newpkt) {
        printf("Error, Failed to alloc buf for packet!\n");
        goto __ERROR;
    }

    //Create the FIFO buffer for the audio samples to be encoded.
    fifo = av_audio_fifo_alloc(c_ctx->sample_fmt, c_ctx->channels, 1);
    if (! fifo) {
        printf("Error, Failed to alloc fifo!\n");
        goto __ERROR;
    }

    // 分配重采样输入/输出缓冲区
    alloc_data_4_resample(&src_data, &src_linesize, &dst_data, &dst_linesize);

    while (1) {
        const int frame_size = c_ctx->frame_size;
        static bool finished = false;

        while (av_audio_fifo_size(fifo) < frame_size) {
            cout << "fifo size:" << av_audio_fifo_size(fifo)
                 << "frame_size:" << frame_size << endl;
            //read frame form device
            ret = av_read_frame(fmt_ctx, &pkt);
            if (ret == 0) {
                memcpy(static_cast<void *>(src_data[0]), pkt.data, static_cast<size_t>(pkt.size));

                //重采样
                swr_convert(swr_ctx,                                //重采样的上下文
                            dst_data,                               //输出结果缓冲区
                            22050,                                  //每个通道的采样数
                            const_cast<const uint8_t **>(src_data), //输入缓冲区
                            22050);                                 //输入单个通道的采样数

                if (add_samples_to_fifo(fifo, src_data, 22050)) {
                    goto __ERROR;
                }
            }

            if (count++ >= 10) {
                finished = true;
                break;
            }
        }

        while (av_audio_fifo_size(fifo) >= frame_size ||
               (finished && av_audio_fifo_size(fifo) > 0)) {
            if(read_fifo_and_encode(fifo, fmt_ctx, c_ctx, frame)) {
                goto __ERROR;
            }
            encode(c_ctx, frame, newpkt, outfile);
        }

        if (finished) {
            //强制将编码器缓冲区中的音频进行编码输出
            encode(c_ctx, nullptr, newpkt, outfile);
            break;
        }
    }

__ERROR:
    //释放AVFrame 和 AVPacket
    if (frame) {
        av_frame_free(&frame);
    }

    if (newpkt) {
        av_packet_free(&newpkt);
    }

    if (fifo) {
        av_audio_fifo_free(fifo);
    }

    //释放重采样缓冲区
    free_data_4_resample(src_data, dst_data);
}

void capture_audio()
{
    vector<string> vecDeviceName;

    AVFormatContext *fmt_ctx = nullptr;
    AVCodecContext *c_ctx = nullptr;
    SwrContext *swr_ctx = nullptr;

    //create file
    //FILE *outfile = fopen("D:/Study/ffmpeg/av_base/audio.pcm", "wb");
    FILE *outfile = fopen("D:/Study/ffmpeg/av_base/audio.aac", "wb");
    if (! outfile) {
        printf("Error, Failed to open file!\n");
        goto __ERROR;
    }

    //register audio device
    avdevice_register_all();

    //获取mic设备的设备名
    vecDeviceName = get_audio_device_name();
    if (vecDeviceName.size() <= 0){
        printf("not find audio input device.\n");
        goto __ERROR;
    }

    //打开设备
    fmt_ctx = open_dev(vecDeviceName[0]);
    if (! fmt_ctx) {
        printf("Error, Failed to open device!\n");
        goto __ERROR;
    }

    //打开编码器上下文
    c_ctx = open_coder();
    if (! c_ctx) {
        printf("Error, Failed to open coder!\n");
        goto __ERROR;
    }

    //初始化重采样上下文
    swr_ctx = init_swr_ctx();
    if (! swr_ctx) {
        printf("Error, Failed to alloc swr_ctx!\n");
        goto __ERROR;
    }

    //encode
    read_data_and_encode(fmt_ctx, c_ctx, swr_ctx, outfile);

__ERROR:
    //释放重采样的上下文
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }

    if (c_ctx) {
        avcodec_free_context(&c_ctx);
    }
    //close device and release ctx
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }

    //close file
    if (outfile) {
        fclose(outfile);
    }

    av_log(nullptr, AV_LOG_DEBUG, "finish!\n");

    return;
}

int main()
{
    capture_audio();

    return 0;
}
