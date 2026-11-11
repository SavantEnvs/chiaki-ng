// Fuzzes the H.264/H.265 SPS and slice-header bitstream parser.
// Layout: [codec][header_len][header bytes][slice bytes...]
#include <chiaki/bitstream.h>
#include "common.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if(size < 2)
		return 0;
	ChiakiCodec codec = (ChiakiCodec)(data[0] % 3);
	size_t header_len = data[1];
	data += 2;
	size -= 2;
	if(header_len > size)
		header_len = size;

	ChiakiLog log;
	mh_log_init(&log);
	ChiakiBitstream bs;
	chiaki_bitstream_init(&bs, &log, codec);

	uint8_t *header = mh_dup(data, header_len);
	size_t slice_len = size - header_len;
	uint8_t *slice = mh_dup(data + header_len, slice_len);
	if(!header || !slice)
	{
		free(header);
		free(slice);
		return 0;
	}

	if(chiaki_bitstream_header(&bs, header, (unsigned)header_len))
	{
		ChiakiBitstreamSlice out;
		if(chiaki_bitstream_slice(&bs, slice, (unsigned)slice_len, &out))
		{
			volatile unsigned sink = out.reference_frame + out.slice_type;
			(void)sink;
			for(unsigned ref = 0; ref < 3; ref++)
			{
				if(chiaki_bitstream_slice_set_reference_frame(&bs, slice, (unsigned)slice_len, ref))
					chiaki_bitstream_slice(&bs, slice, (unsigned)slice_len, &out);
			}
		}
	}
	else
	{
		// slice parsing without a parsed SPS is a state the library allows
		ChiakiBitstreamSlice out;
		chiaki_bitstream_slice(&bs, slice, (unsigned)slice_len, &out);
	}
	free(header);
	free(slice);
	return 0;
}
