#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>

#define MAX_PCE_SIZE 320
#define ADTS_HEADER_SIZE 7

typedef struct {
	int byte_offset;
	int bit_offset;
	uint8_t *buf;
} PutBitContext;

typedef struct ADTSContext {
	AVClass *class;
	int write_adts;
	int objecttype;
	int sample_rate_index;
	int channel_conf;
	int pce_size;
	int apetag;
	int id3v2tag;
	int mpeg_id;
	uint8_t pce_data[MAX_PCE_SIZE];
} ADTSContext;

void init_put_bits(PutBitContext *pb, uint8_t *buf, int size)
{
	memset(pb, 0, sizeof(PutBitContext));
	memset(buf, 0, size);
	pb->buf = buf;
}

void put_bits(PutBitContext *pb, int num, int value)
{
	int write_bits;
	int bits;

	for (int i = num - 1; i >= 0; i--) {
		num--;
		pb->buf[pb->byte_offset] = ((value >> num) | (pb->buf[pb->byte_offset] << 1));
		value &= (~(1 << num));
		pb->bit_offset++;
		if ((pb->bit_offset % 8) == 0) {
			pb->bit_offset = 0;
			pb->byte_offset++;
		}
	}
}

int ff_adts_write_frame_header(uint8_t *buf, int size, int pce_size)
{
	PutBitContext pb;

	init_put_bits(&pb, buf, ADTS_HEADER_SIZE);

	/* adts_fixed_header */
	put_bits(&pb, 12, 0xfff);   // syncword
	put_bits(&pb, 1, 0);        // ID
	put_bits(&pb, 2, 0);        // layer
	put_bits(&pb, 1, 1);        // protection_absent
	put_bits(&pb, 2, 1); 		// profile_objecttype
	put_bits(&pb, 4, 4);
	put_bits(&pb, 1, 0);        // private_bit
	put_bits(&pb, 3, 2); 		// channel_configuration
	put_bits(&pb, 1, 0);        // original_copy
	put_bits(&pb, 1, 0);        // home

	/* adts_variable_header */
	put_bits(&pb, 1, 0);        // copyright_identification_bit
	put_bits(&pb, 1, 0);        // copyright_identification_start
	put_bits(&pb, 13, ADTS_HEADER_SIZE + size + pce_size); // aac_frame_length
	put_bits(&pb, 11, 0x7ff);   // adts_buffer_fullness
	put_bits(&pb, 2, 0);        // number_of_raw_data_blocks_in_frame

	return 0;
}

int main(int argc, char *argv[])
{
	int ret;
	int audio_index;
	int len;
	char *src = NULL;
	char *dst = NULL;
	char adts_header_buf[ADTS_HEADER_SIZE];

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

	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't find the best stream!\n");
		goto __fail;
	}

	audio_index = ret;
	av_init_packet(&pkt);

	while (av_read_frame(fmt_ctx, &pkt) >= 0) {
		if (pkt.stream_index == audio_index) {
			ff_adts_write_frame_header(adts_header_buf, pkt.size, 0);
			fwrite(adts_header_buf, 1, ADTS_HEADER_SIZE, dst_fd);
			len = fwrite(pkt.data, 1, pkt.size, dst_fd);
			if (len != pkt.size) {
				av_log(NULL, AV_LOG_WARNING, "waning, length of data is not equal size of pkt!\n");
			}
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
