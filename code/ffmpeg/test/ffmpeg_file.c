#include "libavformat/avformat.h"
#include "libavutil/log.h"

int main(int argc, char *argv[])
{
	int ret;

	ret = ffurl_move("111.txt", "222.txt");
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to rename\n");
		return -1;
	}

	ret = ffurl_delete("./testfile.txt");
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Failed to delete file testflie.txt\n");
		return -1;
	}

	return 0;
}
