// Fuzzes the video frame reassembly + FEC recovery (frameprocessor.c), driven the way
// videoreceiver.c drives it. Layout: [units_total u8][units_fec u8][is_video u8] then records of
// [unit_index u8][len u8][len bytes]. Flush happens as in chiaki_video_receiver_av_packet.
#include <chiaki/frameprocessor.h>
#include <chiaki/takion.h>
#include "common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size < 3)
		return 0;
	ChiakiLog log;
	mh_log_init(&log);
	ChiakiFrameProcessor fp;
	chiaki_frame_processor_init(&fp, &log);

	uint16_t total = data[0];
	uint16_t fec = data[1];
	bool is_video = data[2] & 1;
	data += 3;
	size -= 3;

	bool first = true;
	while(size >= 2)
	{
		uint8_t unit_index = data[0];
		size_t len = data[1];
		data += 2;
		size -= 2;
		if(len > size)
			len = size;
		uint8_t *buf = mh_dup(data, len);
		data += len;
		size -= len;
		if(!buf)
			break;

		ChiakiTakionAVPacket packet;
		memset(&packet, 0, sizeof(packet));
		packet.units_in_frame_total = total;
		packet.units_in_frame_fec = fec;
		packet.is_video = is_video;
		packet.unit_index = unit_index;
		packet.data = buf;
		packet.data_size = len;

		ChiakiErrorCode err = CHIAKI_ERR_SUCCESS;
		if(first)
		{
			err = chiaki_frame_processor_alloc_frame(&fp, &packet);
			first = false;
		}
		if(err == CHIAKI_ERR_SUCCESS)
		{
			chiaki_frame_processor_put_unit(&fp, &packet);
			if(chiaki_frame_processor_flush_possible(&fp) || packet.unit_index == packet.units_in_frame_total - 1)
			{
				uint8_t *frame = NULL;
				size_t frame_size = 0;
				if(chiaki_frame_processor_flush(&fp, &frame, &frame_size) != CHIAKI_FRAME_PROCESSOR_FLUSH_RESULT_FAILED && frame)
				{
					size_t sum = 0;
					for(size_t i = 0; i < frame_size; i++)
						sum += frame[i];
					volatile size_t sink = sum;
					(void)sink;
				}
			}
		}
		free(buf);
		if(err != CHIAKI_ERR_SUCCESS)
			break;
	}
	chiaki_frame_processor_fini(&fp);
	return 0;
}
