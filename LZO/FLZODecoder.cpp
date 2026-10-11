/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include "FLZODecoder.h"

extern "C" {
#include "minilzo.h"
}

std::size_t FLZODecoder::Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const
{
	lzo_uint outLen = Len32(out.size());
	const int r = lzo1x_decompress_safe(in.data(), Len32(in.size()), out.data(), &outLen, nullptr);
	return (r == LZO_E_OK || r == LZO_E_INPUT_NOT_CONSUMED) ? outLen : 0;
}
