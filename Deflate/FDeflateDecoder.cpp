/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include "FDeflateDecoder.h"

extern "C" {
#include "puff.h"
}

std::size_t FDeflateDecoder::Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const
{
	unsigned long outLen = Len32(out.size());
	unsigned long inLen = Len32(in.size());
	// puff() never writes to its source; its 2003 prototype just lacks the const
	const int ret = puff(out.data(), &outLen, const_cast<std::uint8_t*>(in.data()), &inLen);
	return ret == 0 ? outLen : 0;
}
