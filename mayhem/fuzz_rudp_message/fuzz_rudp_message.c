// Fuzzes the RUDP message decoder used for Remote Play over the internet (static
// chiaki_rudp_message_parse, including its recursive sub-message handling).
#include "../../lib/src/remote/rudp.c"
#include "common.h"

static size_t walk(RudpMessage *m)
{
	size_t sum = 0;
	for(size_t i = 0; i < m->data_size; i++)
		sum += m->data[i];
	if(m->subMessage)
		sum += walk(m->subMessage);
	return sum;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	// chiaki_rudp_select_recv()/chiaki_rudp_recv_only() only parse datagrams with received_sz > 8.
	if(size <= 8)
		return -1;
	uint8_t *buf = mh_dup(data, size);
	if(!buf)
		return 0;
	RudpMessage message;
	if(chiaki_rudp_message_parse(buf, size, &message) == CHIAKI_ERR_SUCCESS)
	{
		volatile size_t sink = walk(&message);
		(void)sink;
	}
	chiaki_rudp_message_pointers_free(&message);
	free(buf);
	return 0;
}
