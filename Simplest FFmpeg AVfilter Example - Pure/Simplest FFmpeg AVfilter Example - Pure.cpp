// Simplest FFmpeg AVfilter Example - Pure.cpp : 定义控制台应用程序的入口点。
//

/**
* 最简单的基于 FFmpeg 的 AVFilter 例子 - 纯净版
* Simplest FFmpeg AVfilter Example - Pure
*
* 雷霄骅 Lei Xiaohua
* leixiaohua1020@126.com
* 中国传媒大学/数字电视技术
* Communication University of China / Digital TV Technology
* http://blog.csdn.net/leixiaohua1020
*
* 修改：
* 刘文晨 Liu Wenchen
* 812288728@qq.com
* 电子科技大学/电子信息
* University of Electronic Science and Technology of China / Electronic and Information Science
* https://blog.csdn.net/ProgramNovice
*
* 本程序使用 FFmpeg 的 AVfilter 实现了 YUV 像素数据的滤镜处理功能。
* 可以给 YUV 数据添加各种特效功能。
* 是最简单的 FFmpeg 的 AVFilter 方面的教程。
* 适合 FFmpeg 的初学者。
*
* This software uses FFmpeg's AVFilter to process YUV raw data.
* It can add many excellent effect to YUV data.
* It's the simplest example based on FFmpeg's AVFilter.
* Suitable for beginner of FFmpeg
*
*/

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

// 解决报错：无法解析的外部符号 __imp__fprintf，该符号在函数 _ShowError 中被引用
#pragma comment(lib, "legacy_stdio_definitions.lib")
extern "C"
{
	// 解决报错：无法解析的外部符号 __imp____iob_func，该符号在函数 _ShowError 中被引用
	FILE __iob_func[3] = { *stdin, *stdout, *stderr };
}

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C"
{
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif

AVFilterContext *buffersrc_ctx;
AVFilterContext *buffersink_ctx;
AVFilterGraph *filter_graph;

static int video_stream_index = -1;
const int in_width = 480;
const int in_height = 272;

// 功能：创建配置一个滤镜图，在后续滤镜处理中，可以往此滤镜图输入数据并从滤镜图获得输出数据
static int init_filters(const char* filter_descr)
{
	int ret;
	// args 是 buffersrc 滤镜的参数
	char args[512];

	AVFilter *buffersrc = avfilter_get_by_name("buffer");
	AVFilter *buffersink = avfilter_get_by_name("ffbuffersink");
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs = avfilter_inout_alloc();
	enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
	AVBufferSinkParams *buffersink_params;

	// 为 FilterGraph 分配内存
	filter_graph = avfilter_graph_alloc();

	// buffer video source: the decoded frames from the decoder will be inserted here
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		in_width, in_height, AV_PIX_FMT_YUV420P,
		1, 25, 1, 1);

	// 创建并向 FilterGraph 中添加一个 Filter
	ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
		args, NULL, filter_graph);
	if (ret < 0)
	{
		printf("Can't create buffer source.\n");
		return ret;
	}

	// buffer video sink: to terminate the filter chain
	buffersink_params = av_buffersink_params_alloc();
	buffersink_params->pixel_fmts = pix_fmts;
	// 创建并向 FilterGraph 中添加一个 Filter
	ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
		NULL, buffersink_params, filter_graph);
	av_free(buffersink_params);
	if (ret < 0)
	{
		printf("Can't create buffer sink.\n");
		return ret;
	}

	// endpoints for the filter graph
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	// 将一串通过字符串描述的 Graph 添加到 FilterGraph 中
	ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, NULL);
	if (ret < 0)
	{
		return ret;
	}

	// 检查 FilterGraph 的配置
	// check validity and configure all the links and formats in the graph
	ret = avfilter_graph_config(filter_graph, NULL);
	if (ret < 0)
	{
		return ret;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int ret;
	AVFrame* frame_in;
	AVFrame* frame_out;
	unsigned char* frame_buffer_in;
	unsigned char* frame_buffer_out;

	// 帧计数器
	int frame_cnt = 0;

	// 输入 YUV 文件
	FILE* fp_in = fopen("sintel_480x272_yuv420p.yuv", "rb+");
	if (fp_in == NULL) {
		printf("Can't open input file.\n");
		return -1;
	}

	// 输出 YUV 文件
	FILE* fp_out = fopen("output.yuv", "wb+");
	if (fp_out == NULL) {
		printf("Can't open output file.\n");
		return -1;
	}

	// const char *filter_descr = "lutyuv='u=128:v=128'";
	// const char *filter_descr = "boxblur";
	// const char *filter_descr = "hflip";
	// const char *filter_descr = "hue='h=60:s=-3'";
	// const char *filter_descr = "crop=1/2*in_w:1/2*in_h";
	// const char *filter_descr = "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5";
	const char *filter_descr = "drawtext=fontfile=arial.ttf:fontcolor=green:fontsize=30:text='UestcXiye'";

	// 注册所有 AVFilter
	avfilter_register_all();

	ret = init_filters(filter_descr);
	if (ret < 0)
	{
		goto end;
	}

	frame_in = av_frame_alloc();
	frame_buffer_in = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width, in_height, 1));
	av_image_fill_arrays(frame_in->data, frame_in->linesize, frame_buffer_in,
		AV_PIX_FMT_YUV420P, in_width, in_height, 1);

	frame_out = av_frame_alloc();
	frame_buffer_out = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, in_width, in_height, 1));
	av_image_fill_arrays(frame_out->data, frame_out->linesize, frame_buffer_out,
		AV_PIX_FMT_YUV420P, in_width, in_height, 1);

	frame_in->width = in_width;
	frame_in->height = in_height;
	frame_in->format = AV_PIX_FMT_YUV420P;

	while (1)
	{
		if (fread(frame_buffer_in, 1, in_width * in_height * 3 / 2, fp_in) != in_width * in_height * 3 / 2)
		{
			break;
		}
		// Input Y, U, V
		frame_in->data[0] = frame_buffer_in;
		frame_in->data[1] = frame_buffer_in + in_width * in_height;
		frame_in->data[2] = frame_buffer_in + in_width * in_height * 5 / 4;

		// 向 FilterGraph 中加入一个 AVFrame
		if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0)
		{
			printf("Error while feeding the filtergraph.\n");
			break;
		}

		// 从 FilterGraph 中取出一个 AVFrame
		ret = av_buffersink_get_frame(buffersink_ctx, frame_out);
		if (ret < 0)
		{
			break;
		}

		printf("Process %d frame.\n", ++frame_cnt);

		// Output Y, U, V
		if (frame_out->format == AV_PIX_FMT_YUV420P)
		{
			// Y, U, V
			for (int i = 0; i < frame_out->height; i++)
			{
				fwrite(frame_out->data[0] + frame_out->linesize[0] * i, 1, frame_out->width, fp_out);
			}
			for (int i = 0; i < frame_out->height / 2; i++)
			{
				fwrite(frame_out->data[1] + frame_out->linesize[1] * i, 1, frame_out->width / 2, fp_out);
			}
			for (int i = 0; i < frame_out->height / 2; i++)
			{
				fwrite(frame_out->data[2] + frame_out->linesize[2] * i, 1, frame_out->width / 2, fp_out);
			}
		}

		av_frame_unref(frame_out);
	}

	fclose(fp_in);
	fclose(fp_out);

end:
	av_frame_free(&frame_in);
	av_frame_free(&frame_out);
	avfilter_graph_free(&filter_graph);

	if (ret < 0 && ret != AVERROR_EOF)
	{
		char buf[1024];
		av_strerror(ret, buf, sizeof(buf));
		printf("Error occurred: %s.\n", buf);
		return -1;
	}

	system("pause");
	return 0;
}

