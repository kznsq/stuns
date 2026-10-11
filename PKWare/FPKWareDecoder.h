/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FPKWAREDECODER_H
#define FPKWAREDECODER_H

#include "../FBaseDecoder.h"

// PKWare Data Compression Library (implode) through blast
class FPKWareDecoder final : public FBaseDecoder
{
public:
	FPKWareDecoder() : FBaseDecoder("PKWare") {}
	std::size_t Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const override;
};

#endif
