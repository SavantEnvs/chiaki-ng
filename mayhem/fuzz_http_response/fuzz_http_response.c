// Fuzzes the HTTP-style header/response parser used for the PS4/PS5 registration, wakeup and
// discovery replies. Byte 0 picks the entry point: even = bare header block, odd = full response.
#include <chiaki/http.h>
#include "common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size < 1)
		return 0;
	int whole_response = data[0] & 1;
	size_t buf_size = size - 1;
	char *buf = (char *)mh_dup(data + 1, buf_size);
	if(!buf)
		return 0;

	ChiakiHttpHeader *headers = NULL;
	if(whole_response)
	{
		ChiakiHttpResponse response;
		memset(&response, 0, sizeof(response));
		if(chiaki_http_response_parse(&response, buf, buf_size) == CHIAKI_ERR_SUCCESS)
		{
			headers = response.headers;
			volatile int code = response.code;
			(void)code;
		}
	}
	else
	{
		if(chiaki_http_header_parse(&headers, buf, buf_size) != CHIAKI_ERR_SUCCESS)
			headers = NULL;
	}

	size_t total = 0;
	for(ChiakiHttpHeader *h = headers; h; h = h->next)
		total += strlen(h->key) + strlen(h->value);
	volatile size_t sink = total;
	(void)sink;
	chiaki_http_header_free(headers);
	free(buf);
	return 0;
}
