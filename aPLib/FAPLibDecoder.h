/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FAPLIBDECODER_H
#define FAPLIBDECODER_H

#include "../FBaseDecoder.h"

// Raw aPLib stream (no "AP32" header); the stream must end with its end marker.
//
// The end marker is short and easy to hit by chance, so random data and executables
// give many false hits: this decoder is off by default.
class FAPLibDecoder final : public FBaseDecoder
{
public:
	FAPLibDecoder() : FBaseDecoder("aPLib") {}
	std::size_t Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const override;
};

#endif
