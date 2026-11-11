// Fuzzes the three Takion AV packet header parsers (protocol versions 7, 9 and 12).
// Byte 0 selects the version, the rest is the datagram.
#include <chiaki/takion.h>
#include <chiaki/gkcrypt.h>
#include "common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size < 1)
		return 0;
	unsigned version = data[0] % 3;
	size_t buf_size = size - 1;
	uint8_t *buf = mh_dup(data + 1, buf_size);
	if(!buf)
		return 0;

	ChiakiKeyState key_state;
	chiaki_key_state_init(&key_state);
	ChiakiTakionAVPacket packet;
	memset(&packet, 0, sizeof(packet));
	ChiakiErrorCode err;
	switch(version)
	{
		case 0: err = chiaki_takion_v7_av_packet_parse(&packet, &key_state, buf, buf_size); break;
		case 1: err = chiaki_takion_v9_av_packet_parse(&packet, &key_state, buf, buf_size); break;
		default: err = chiaki_takion_v12_av_packet_parse(&packet, &key_state, buf, buf_size); break;
	}
	(void)err;
	free(buf);
	return 0;
}
