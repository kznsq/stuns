/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FDEFLATEDECODER_H
#define FDEFLATEDECODER_H

#include "../FBaseDecoder.h"

// raw Deflate (RFC 1951) through puff
class FDeflateDecoder final : public FBaseDecoder
{
public:
	FDeflateDecoder() : FBaseDecoder("Deflate") {}
	std::size_t Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const override;
};

#endif
