#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>

#ifndef AV_RB16
	#define AV_RB16(x)					\
	((((const uint8_t *)(x))[0] << 8) |	\
	((const uint8_t *)(x))[1])
#endif

static int add_start_code(AVPacket *out, const uint8_t *buf, int size)
{
	int start_code_size = 4;
	int ret;
	char start_code[4] = {0x00,0x00, 0x00, 0x01};

	ret = av_grow_packet(out, size + start_code_size);
	if (ret < 0) {
		return ret;
	}

	memcpy(out->data, start_code, start_code_size);
	memcpy(&out->data[4], buf, size);
	out->size = size + start_code_size;

	return 0;
}

static int add_SPS_PPS(const AVStream *stream, AVPacket *out_extradata)
{
	const uint8_t *extradata = NULL;
	const uint8_t *extradata_end = NULL;
	int extradata_size;
	uint8_t unit_nb = 0;
	uint16_t unit_size = 0;
	int err;
	const uint8_t nalu_header[4] = {0, 0, 0, 1};
	int total_size;
	int sps_done = 0;

	/**
	 * AVCC
	 * bits
	 *  8   version ( always 0x01 )
	 *  8   avc profile ( sps[0][1] )
	 *  8   avc compatibility ( sps[0][2] )
	 *  8   avc level ( sps[0][3] )
	 *  6   reserved ( all bits on )
	 *  2   NALULengthSizeMinusOne    // 这个值是（前缀长度-1），值如果是3，那前缀就是4，因为4-1=3
	 *  3   reserved ( all bits on )
	 *  5   number of SPS NALUs (usually 1)
	 *
	 *  repeated once per SPS:
	 *  16     SPS size
	 *
	 *  variable   SPS NALU data
	 *  8   number of PPS NALUs (usually 1)
	 *  repeated once per PPS
	 *  16    PPS size
	 *  variable PPS NALU data
	 */

	extradata = stream->codecpar->extradata;
	extradata_size = stream->codecpar->extradata_size;
	extradata_end = extradata + extradata_size;
	
	//extradata前5字节无作用，跳过
	extradata += 5;
	unit_nb = *extradata++ & 0x1F;
	if (!unit_nb) {
		goto PPS;
	}

	while (unit_nb--) {
		unit_size = AV_RB16(extradata);
		total_size  = unit_size + 4;
		if (total_size > INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE) {
			av_log(NULL, AV_LOG_ERROR, "Too big extradeat size, corrupted stream or invalid MP4/AVCC bitstream\n");
			return AVERROR(EINVAL);
		}

		if (extradata + 2 + unit_size > extradata_end) {
			av_log(NULL, AV_LOG_ERROR, "SPS is not contained in global extradata, corrupted stream or invalid MP4/AVCC bitstream\n");
			return AVERROR(EINVAL);
		}
		
		if ((err = av_reallocp(&out_extradata->data, total_size + AV_INPUT_BUFFER_PADDING_SIZE)) < 0)
			return err;

		memcpy(out_extradata, nalu_header, 4);
		memcpy(out_extradata + 4, extradata + 2, unit_size);
		extradata += 2 + unit_size;
	}
PPS:
	if (!unit_nb && !sps_done++) {
		unit_nb = *extradata++;
	}

	if (out_extradata) {
		memset(out_extradata + total_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	}

	return 0;
}

static int h264_mp4_to_annexb(AVFormatContext *fmt_ctx, AVPacket *packet, FILE *dst_fd)
{
	AVPacket *out = NULL;

	const uint8_t *in;
	const uint8_t *in_end;
	int in_size;
	int total_nal_size = 0;
	int nal_size = 0;
	int sps_pps_size = 0;
	uint8_t unit_type;
	int len;

	out = av_packet_alloc();
	if (!out) {
		av_log(NULL, AV_LOG_ERROR, "The out packet alloc failed!\n");
		goto fail;
	}

	in = packet->data;
	in_size = packet->size;
	in_end = in + in_size;

	while (total_nal_size < in_size) {
		// 判断输入packet的数据长度是否大于4字节
		if (in + 4 > in_end) {
			av_log(NULL, AV_LOG_ERROR, "Invalid packet size!\n");
			goto fail;
		}

		nal_size = 0;
		for (int i = 0; i < 4; i++) {
			nal_size += in[i] << ((3 - i) * 8);
		}

		if (nal_size > in_size || nal_size < 0) {
			av_log(NULL, AV_LOG_ERROR, "Invalid nal size!\n");
			goto fail;
		}

		in += 4;
		unit_type = in[0] & 0x1f;

		// prepend only to the first type 5 NAL unit of an IDR picture, if no sps/pps are already present
		if (unit_type == 5) { 
			add_SPS_PPS(fmt_ctx->streams[packet->stream_index], out);
			sps_pps_size = out->size;
			av_grow_packet(out, sps_pps_size + nal_size);
			memcpy(out + sps_pps_size, in, nal_size);
		} else {
			if (add_start_code(out, in, nal_size) < 0) {
				goto fail;
			}
		}

		len = fwrite(out->data, 1, out->size, dst_fd);
		if (len != out->size) {
			av_log(NULL, AV_LOG_DEBUG, "length of writed data isn't equal pkt.size(%d, %d)\n", len, out->size);
		}

		fflush(dst_fd);

		in += nal_size;
		total_nal_size = nal_size + 4;
	}

fail:
	if (out) {
		av_free(out);
	}
}

int main(int argc, char *argv[])
{
	int ret;
	int video_index;
	int len;
	char *src = NULL;
	char *dst = NULL;

	AVFormatContext *fmt_ctx = NULL;
	AVPacket pkt;

	av_log_set_level(AV_LOG_INFO);
	avdevice_register_all();

	if (argc < 3) {
		av_log(NULL, AV_LOG_ERROR, "The count of params should be more than three!\n");
		return -1;
	}

	src = argv[1];
	dst = argv[2];
	if (!src || !dst) {
		av_log(NULL, AV_LOG_ERROR, "src or dst is null\n");
		return -1;
	}

	ret = avformat_open_input(&fmt_ctx, src, NULL, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't open file: %s\n", av_err2str(ret));
		return -1;
	}

	FILE *dst_fd = fopen(dst, "wb");
	if (!dst_fd) {
		av_log(NULL, AV_LOG_ERROR, "Can't open out file!\n");
		goto __fail;
	}

	av_dump_format(fmt_ctx, 0, src, 0);

	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't find the best stream!\n");
		goto __fail;
	}

	video_index = ret;
	av_init_packet(&pkt);

	while (av_read_frame(fmt_ctx, &pkt) >= 0) {
		if (pkt.stream_index == video_index) {
			h264_mp4_to_annexb(fmt_ctx, &pkt, dst_fd);			
		}

		av_packet_unref(&pkt);
	}

__fail:
	if (fmt_ctx) {
		avformat_close_input(&fmt_ctx);
	}

	if (dst_fd) {
		fclose(dst_fd);
	}

	return ret;
}
