// Fuzzes the Takion receive path (static takion_handle_packet): MAC handling, control/data message
// parsing, data reordering, protobuf-typed data dispatch and the video/audio reorder queues.
// takion.c is compiled into the harness to reach the static functions; no socket is involved.
// Layout: [flags][seq u32 LE][ (len u16 LE)(len bytes) ... ]  -> each chunk is one datagram.
#include "../../lib/src/takion.c"
#include "common.h"

static void takion_event_cb(ChiakiTakionEvent *event, void *user)
{
	(void)event;
	(void)user;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size < 5)
		return 0;
	uint8_t flags = data[0];
	uint32_t seq = data[1] | (data[2] << 8) | (data[3] << 16) | ((uint32_t)data[4] << 24);
	data += 5;
	size -= 5;

	ChiakiLog log;
	mh_log_init(&log);
	ChiakiTakion t;
	memset(&t, 0, sizeof(t));
	t.log = &log;
	t.sock = CHIAKI_INVALID_SOCKET;
	t.cb = takion_event_cb;
	t.disable_audio_video = (ChiakiDisableAudioVideo)((flags >> 2) & 3);
	switch(flags & 3)
	{
		case 0: t.version = 7; t.av_packet_parse = chiaki_takion_v7_av_packet_parse; break;
		case 1: t.version = 9; t.av_packet_parse = chiaki_takion_v9_av_packet_parse; break;
		default: t.version = 12; t.av_packet_parse = chiaki_takion_v12_av_packet_parse; break;
	}
	chiaki_key_state_init(&t.key_state);
	if(chiaki_mutex_init(&t.gkcrypt_local_mutex, false) != CHIAKI_ERR_SUCCESS)
		return 0;
	if(chiaki_mutex_init(&t.send_buffer_active_mutex, false) != CHIAKI_ERR_SUCCESS)
		goto err_m1;
	if(chiaki_mutex_init(&t.seq_num_local_mutex, false) != CHIAKI_ERR_SUCCESS)
		goto err_m2;
	if(chiaki_reorder_queue_init_32(&t.data_queue, TAKION_REORDER_QUEUE_SIZE_EXP, seq) != CHIAKI_ERR_SUCCESS)
		goto err_m3;
	chiaki_reorder_queue_set_drop_cb(&t.data_queue, takion_data_drop, &t);
	if(chiaki_takion_send_buffer_init(&t.send_buffer, &t, CHIAKI_TAKION_SEND_BUFFER_SIZE) != CHIAKI_ERR_SUCCESS)
		goto err_queue;
	t.send_buffer_active = true;

	while(size >= 2)
	{
		size_t len = data[0] | (data[1] << 8);
		data += 2;
		size -= 2;
		if(len > size)
			len = size;
		if(len == 0)
			continue;
		uint8_t *buf = mh_dup(data, len);
		data += len;
		size -= len;
		if(!buf)
			break;
		takion_handle_packet(&t, buf, len); // takes ownership of buf
	}

	t.send_buffer_active = false;
	chiaki_takion_send_buffer_fini(&t.send_buffer);
	if(t.video_queue_initialized)
		chiaki_reorder_queue_fini(&t.video_queue);
err_queue:
	chiaki_reorder_queue_fini(&t.data_queue);
err_m3:
	chiaki_mutex_fini(&t.seq_num_local_mutex);
err_m2:
	chiaki_mutex_fini(&t.send_buffer_active_mutex);
err_m1:
	chiaki_mutex_fini(&t.gkcrypt_local_mutex);
	return 0;
}
