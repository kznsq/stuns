/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FLZ4DECODER_H
#define FLZ4DECODER_H

#include "../FBaseDecoder.h"

// Raw LZ4 block (no frame header).
//
// A raw block has no end marker: it simply ends after a literal run. Sequences are
// decoded until the input ends or a sequence is invalid (zero offset, offset before
// the start of the output, output buffer full); the result is everything up to the
// end of the last literal run, where the block may have ended. A block that never
// uses a match is only a copy of its input and is not reported.
class FLZ4Decoder final : public FBaseDecoder
{
public:
	FLZ4Decoder() : FBaseDecoder("LZ4") {}
	std::size_t Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const override;
};

#endif
