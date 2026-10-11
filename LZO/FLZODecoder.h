/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FLZODECODER_H
#define FLZODECODER_H

#include "../FBaseDecoder.h"

// LZO1X through miniLZO
class FLZODecoder final : public FBaseDecoder
{
public:
	FLZODecoder() : FBaseDecoder("LZO") {}
	std::size_t Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const override;
};

#endif
