/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FUCLDECODER_H
#define FUCLDECODER_H

#include "../FBaseDecoder.h"

// UCL NRV2B, NRV2D and NRV2E (8-bit bit buffer variants)
class FUCLDecoder final : public FBaseDecoder
{
public:
	enum class Variant { NRV2B, NRV2D, NRV2E };

	explicit FUCLDecoder(Variant variant);
	std::size_t Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const override;

private:
	Variant Kind;
};

#endif
