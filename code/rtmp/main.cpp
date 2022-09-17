#include <iostream>
#include <unistd.h>

//using namespace std;

//包含ffmpeg头文件
extern "C"
{
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/audio_fifo.h"
#include "librtmp/rtmp.h"
}

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include<winsock2.h>

using std::vector;
using std::string;
using std::shared_ptr;

/*
 * FLV Header size: 9 byte
 * byte 1-3, signature: 'F' 'L' 'V'
 * byte 4, version: 1
 * byte 5, 0-5 bit: 保留, 必须是 0
 * byte 5, 6 bit: 是否有音频 Tag
 * byte 5, 7 bit: 保留，必须是 0
 * byte 5, 8 bit: 是否有视频 Tag
 * byte 6-9, Header Size: 必须是9
 */
static FILE *open_flv(char *flv_name)
{
    FILE *fp = nullptr;

    fp = fopen(flv_name, "rb");
    if (!fp) {
        printf("Failed to open flv: %s", flv_name);
        return fp;
    }

    fseek(fp, 9, SEEK_SET); // 9 字节的 FLV Header
    fseek(fp, 4, SEEK_CUR); // 4 字节的 PreTabSize

    return fp;
}

static RTMP *conect_rtmp_server(char *rtmpaddr)
{
    RTMP *rtmp = nullptr;

    //1. 创建RTMP对象，并进行初始化
    rtmp = RTMP_Alloc();
    if (!rtmp) {
        printf("NO Memory, Failed to alloc RTMP object!\n");
        goto __ERROR;
    }
    RTMP_Init(rtmp);

    //2. 设置RTMP服务器地址，以及连接超时时间
    rtmp->Link.timeout = 10;
    RTMP_SetupURL(rtmp, rtmpaddr);

    //3. 建立连接
//#ifdef WIN32
        // Initialize Winsock
        int iResult;
        WSADATA wsaData;
        iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != 0) {
            std::cout << "WSAStartup failed: " << iResult << std::endl;
            goto __ERROR;
        }
//#endif

    if (!RTMP_Connect(rtmp, nullptr)) {
        printf("Failed to Connect RTMP Server!\n");
        goto __ERROR;
    }

    //4. 设置是推流还是拉流
    //如果设置了该开关，就是推流，否则就是拉流
    RTMP_EnableWrite(rtmp);

    //5. create stream
    RTMP_ConnectStream(rtmp, 0);

    return rtmp;

__ERROR:

    //释放资源
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }

    return nullptr;
}

//分配RTMPPacket空间
static RTMPPacket *alloc_packet(void)
{
    RTMPPacket *pack = nullptr;

    pack = (RTMPPacket *)malloc(sizeof(RTMPPacket));
    if (!pack) {
        printf("No Memory, Failed to alloc RTMPPacket!\n");
        return pack;
    }

    RTMPPacket_Alloc(pack, 150 * 1024);
    RTMPPacket_Reset(pack);

    pack->m_hasAbsTimestamp = 0;
    pack->m_nChannel = 0x4;

    return pack;
}

static void read_u8(FILE *fp, unsigned int *u8)
{
    fread(u8, 1, 1, fp);
    return;
}

static void read_u24(FILE *fp, unsigned int *u24)
{
    unsigned int tmp;

    fread(&tmp, 1, 3, fp);
    *u24 = ((tmp >> 16) & 0xFF) | ((tmp << 16) & 0xFF0000) | (tmp & 0xFF00);
 
    return;
}

static void read_u32(FILE *fp, unsigned int *u32)
{
    unsigned int tmp;

    fread(&tmp, 1, 4, fp);
    *u32 = ((tmp >> 24) & 0xFF) | ((tmp << 24) & 0xFF000000) | ((tmp >> 8 ) & 0xFF00) | ((tmp << 8) & 0xFF0000);
    return;
}

/**
 * @brief 从flv文件中读取数据
 * 
 * @param fp [in]：flv file
 * @param packet [out]: the data from flv
 * 
 * @return 0: success, -1: failure 
 */
static int read_data(FILE *fp, RTMPPacket **packet)
{
    /*
     * tag header
     * byte 1, TT(Tag Type), 0x08 音频, 0x09 视频, 0x12 script
     * byte 2-4, Tag body的长度，preTagSize - Tag Header size
     * byte 5-7, 时间戳，单位是毫秒；script 它的时间戳是0
     * byte 8, 扩展时间戳。真正的时间戳结构 [扩展，时间戳] 一共是4字节。
     * byte 9-11，streamID，0
     */
    int ret = -1;
    size_t datasize = 0;

    unsigned int tt;
    unsigned int tag_data_size;
    unsigned int ts;
    unsigned int streamid;
    unsigned int frame_type;
    unsigned int previous_tag_len;
    static unsigned int count = 0;

    read_u8(fp, &tt);
    read_u24(fp, &tag_data_size);
    read_u32(fp, &ts);
    read_u24(fp, &streamid);
    count++;
    datasize = fread((*packet)->m_body, 1, tag_data_size, fp);
    if (tag_data_size != datasize) {
        printf("Failed to read tag body from flv, (datasize=%zu:tds=%d) count:%d\n",
                datasize, tag_data_size, count);
    }
 
    read_u32(fp, &previous_tag_len);

    //设置packet数据
    (*packet)->m_headerType = RTMP_PACKET_SIZE_LARGE;
    (*packet)->m_nTimeStamp = ts;
    (*packet)->m_packetType = tt;
    (*packet)->m_nBodySize = tag_data_size;

    ret = 0;

    return ret;
}

//向流媒体服务器推流
static void send_data(FILE *fp, RTMP *rtmp)
{
    RTMPPacket *packet = nullptr;
    unsigned int pre_ts = 0;
    unsigned int diff = 0;

    //1. 创建 RTMPPacket 对象
    packet = alloc_packet();

    while (1) {
        //2. 从flv文件中读取数据
        read_data(fp, &packet);

        //3. 判断RTMP连接是否正常
        if (!RTMP_IsConnected(rtmp)) {
            printf("Disconnect...\n");
            break;
        }
        usleep(10000);
        //diff = packet->m_nBodySize - pre_ts;
        //printf("diff: %d", packet->m_nBodySize);
        //usleep(packet->m_nBodySize);

        //4. 发送数据
        int ret = RTMP_SendPacket(rtmp, packet, 0);
        //printf("RTMP_SendPacket %d", ret);
        pre_ts = packet->m_nTimeStamp;
    }

    return;
}

void publish_stream()
{
    char *flv = nullptr;
    char * rtmpaddr = nullptr;

    flv = "./out.flv";
    rtmpaddr = "rtmp://106.52.30.12/live/livestream";
    //1. 读 flv 文件
    FILE *fp = open_flv(flv);

    //2. 连接 RTMP 服务器
    RTMP *rtmp = conect_rtmp_server(rtmpaddr);

    //3. publish audio/video data
    send_data(fp, rtmp);

    return;
}

int main()
{
    publish_stream();
    while (1)
    {
        /* code */
    }
    
    return 0;
}
