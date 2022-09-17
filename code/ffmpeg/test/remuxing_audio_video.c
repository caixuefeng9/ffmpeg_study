#include <stdbool.h>
#include <libavutil/timestamp.h>
#include <libavutil/log.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int get_audio_video_stream_from_file(char *src, AVFormatContext **fmt_ctx, enum AVMediaType type)
{
	int ret;
	AVFormatContext *fmt_ctx_audio = NULL;

	ret = avformat_open_input(fmt_ctx, src, NULL, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can,t open file: %s\n", av_err2str(ret));
		return -1;
	}

	ret = avformat_find_stream_info(*fmt_ctx, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to retrieve input stream information\n");
	}

	ret = av_find_best_stream(*fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Can't find the best stream!\n");
		return -1;
	}

	av_dump_format(*fmt_ctx, 0, src, 0);

	return ret;
}

int remuxing_audio_video(char *src1, char *src2, char *output)
{
	int ret;
	int audio_index;
	int video_index;
	AVFormatContext *fmt_ctx_audio = NULL;
	AVFormatContext *fmt_ctx_video = NULL;	
	AVFormatContext *ofmt_ctx = NULL;
	AVStream *out_stream_audio = NULL;
	AVStream *out_stream_video = NULL;
	AVPacket *pkt = NULL;

	audio_index = get_audio_video_stream_from_file(src1, &fmt_ctx_audio, AVMEDIA_TYPE_AUDIO);
	video_index = get_audio_video_stream_from_file(src2, &fmt_ctx_video, AVMEDIA_TYPE_VIDEO);

	if ((audio_index < 0) | (video_index < 0)) {
		av_log(NULL, AV_LOG_ERROR, "Can't find the best audio or video stream!\n");
		goto __fail;
	}

	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output);
	if (!ofmt_ctx) {
		av_log(NULL, AV_LOG_ERROR, "Can't create output context!\n");
		goto __fail;
	}

	out_stream_video = avformat_new_stream(ofmt_ctx, NULL);
	out_stream_audio = avformat_new_stream(ofmt_ctx, NULL);
	if (!out_stream_audio | !out_stream_video) {
		av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream!\n");
		goto __fail;
	}

	ret = avcodec_parameters_copy(out_stream_audio->codecpar, fmt_ctx_audio->streams[audio_index]->codecpar);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to copy audio codec parametes!\n");
		goto __fail;
	}
	out_stream_audio->codecpar->codec_tag = 0;

	ret = avcodec_parameters_copy(out_stream_video->codecpar, fmt_ctx_video->streams[video_index]->codecpar);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to copy video codec parametes!\n");
		goto __fail;
	}
	out_stream_video->codecpar->codec_tag = 0;

	av_dump_format(ofmt_ctx, 0, output, 1);

	if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, output, AVIO_FLAG_WRITE);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Can't open output file!\n");
			goto __fail;
		}
	}

	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to write header!\n");
		goto __fail;
	}

	int frame_index = 0;
	int64_t cur_audio_pts = 0;
	int64_t cur_video_pts = 0;

	pkt = av_packet_alloc();
	if (!pkt) {
		av_log(NULL, AV_LOG_ERROR, "Can't allocate AVPacket!\n");
		goto __fail;
	}
	av_log(NULL, AV_LOG_ERROR, "line:%d\n", __LINE__);

	int count = 0;
	while (1) {
		AVStream *in_stream, *out_stream;
		AVFormatContext *ifmt_ctx;
		int out_index = -1;

		count++;

		if (av_compare_ts(cur_video_pts, fmt_ctx_video->streams[video_index]->time_base,
					cur_audio_pts, fmt_ctx_audio->streams[audio_index]->time_base) < 0) {
			ifmt_ctx = fmt_ctx_video;
			in_stream = fmt_ctx_video->streams[video_index];
			out_stream = out_stream_video;
			out_index = video_index;
		} else {
			ifmt_ctx = fmt_ctx_audio;
			in_stream = fmt_ctx_audio->streams[audio_index];
			out_stream = out_stream_audio;
			out_index = audio_index;
		}

		if (av_read_frame(ifmt_ctx, pkt) < 0) {
			break;
		}

		if (pkt->stream_index != out_index) {
			continue;
		}

		//FIX：No PTS (Example: Raw H.264)
		//Simple Write PTS
		if(pkt->pts == AV_NOPTS_VALUE){
			//Write PTS
			AVRational time_base1 = in_stream->time_base;
			//Duration between 2 frames (us)
			av_log(NULL, AV_LOG_ERROR, "AV_TIME_BASE=%d,av_q2d=%d(num=%d, den=%d)\n",
					AV_TIME_BASE,
					av_q2d(in_stream->r_frame_rate),
					in_stream->r_frame_rate.num,
					in_stream->r_frame_rate.den);

			int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
			//Parameters
			pkt->pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
			pkt->dts = pkt->pts;
			pkt->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
			frame_index++;
		}

		if (out_index == video_index) {
			cur_video_pts = pkt->pts;
		} else {
			cur_audio_pts = pkt->pts;
		}

		av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
		pkt->pos = -1;
		pkt->stream_index = out_index;

		ret = av_interleaved_write_frame(ofmt_ctx, pkt);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error muxing packet\n");
			//break;
			continue;
		}

		av_packet_unref(pkt);
	}

	av_write_trailer(ofmt_ctx);

__fail:
	av_packet_free(&pkt);

	if (fmt_ctx_audio) {
		avformat_close_input(&fmt_ctx_audio);
	}

	if (fmt_ctx_video) {
		avformat_close_input(&fmt_ctx_video);
	}

	/* close output */
	if (ofmt_ctx && !(ofmt_ctx->flags & AVFMT_NOFILE))
		avio_closep(&ofmt_ctx->pb);

	if (ofmt_ctx) {
		avformat_close_input(&ofmt_ctx);
	}
}

int main(int argc, char *argv[])
{
	char *src1 = NULL;
	char *src2 = NULL;
	char *dst = NULL;

	av_log_set_level(AV_LOG_INFO);
	avdevice_register_all();

	if (argc < 4) {
		av_log(NULL, AV_LOG_ERROR, "The count of params should be more than three!\n");
		return -1;
	}

	src1 = argv[1];
	src2 = argv[2];
	dst = argv[3];

	if (!src1 || !src2 || !dst) {
		av_log(NULL, AV_LOG_ERROR, "src or dst is null\n");
		return -1;
	}

	return remuxing_audio_video(src1, src2, dst);
}
