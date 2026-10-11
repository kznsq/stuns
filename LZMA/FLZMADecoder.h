/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FLZMADECODER_H
#define FLZMADECODER_H

#include "../FBaseDecoder.h"

// Raw LZMA stream (no .lzma / .7z header) with the default properties lc=3, lp=0,
// pb=2; the dictionary is the whole output buffer.
//
// Only streams that end with the end marker are reported. Without the marker the end
// of a stream cannot be told from the data after it (the decoder runs on into that
// data), so such streams are not found.
class FLZMADecoder final : public FBaseDecoder
{
public:
	FLZMADecoder() : FBaseDecoder("LZMA") {}
	std::size_t Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const override;
};

#endif
