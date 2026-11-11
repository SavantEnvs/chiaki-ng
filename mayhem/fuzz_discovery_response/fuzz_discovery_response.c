// Fuzzes the PS4/PS5 discovery (SRCH) response parser.
#include <chiaki/discovery.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "common.h"

// exported from lib/src/discovery.c but not declared in any public header
ChiakiErrorCode chiaki_discovery_srch_response_parse(ChiakiDiscoveryHost *response, struct sockaddr *addr, char *addr_buf, size_t addr_buf_size, char *buf, size_t buf_size);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size < 1)
		return 0;
	// discovery_thread_func recv()s into a buffer and NUL-terminates it at buf[n] before parsing.
	char *buf = malloc(size + 1);
	if(!buf)
		return 0;
	memcpy(buf, data, size);
	buf[size] = '\0';

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CHIAKI_DISCOVERY_PORT_PS4);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	char addr_buf[64];

	ChiakiDiscoveryHost host;
	if(chiaki_discovery_srch_response_parse(&host, (struct sockaddr *)&addr, addr_buf, sizeof(addr_buf), buf, size) == CHIAKI_ERR_SUCCESS)
	{
		size_t total = 0;
#define SUM(name) if(host.name) total += strlen(host.name)
		SUM(host_addr); SUM(system_version); SUM(device_discovery_protocol_version); SUM(host_name);
		SUM(host_type); SUM(host_id); SUM(running_app_titleid); SUM(running_app_name);
#undef SUM
		volatile size_t sink = total + host.host_request_port + (size_t)host.state;
		(void)sink;
	}
	free(buf);
	return 0;
}
