// Fuzzes the decrypted registration response payload parser (static regist_parse_response_payload),
// by compiling regist.c into the harness. Byte 0 selects the console generation.
#include "../../lib/src/regist.c"
#include "common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size < 1)
		return 0;
	static const ChiakiTarget targets[] = {
		CHIAKI_TARGET_PS4_UNKNOWN, CHIAKI_TARGET_PS4_8, CHIAKI_TARGET_PS4_9,
		CHIAKI_TARGET_PS4_10, CHIAKI_TARGET_PS5_UNKNOWN, CHIAKI_TARGET_PS5_1
	};
	ChiakiLog log;
	mh_log_init(&log);
	ChiakiRegist regist;
	memset(&regist, 0, sizeof(regist));
	regist.log = &log;
	regist.info.target = targets[data[0] % (sizeof(targets) / sizeof(targets[0]))];

	size_t buf_size = size - 1;
	char *buf = (char *)mh_dup(data + 1, buf_size);
	if(!buf)
		return 0;
	ChiakiRegisteredHost host;
	memset(&host, 0, sizeof(host));
	regist_parse_response_payload(&regist, &host, buf, buf_size);
	free(buf);
	return 0;
}
