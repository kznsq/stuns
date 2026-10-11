/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include "FUCLDecoder.h"

extern "C" {
#include "ucl/ucl.h"
// NRV2E (UCL/n2e_ds.c) is not declared in the UCL 0.92 header
UCL_EXTERN(int)
ucl_nrv2e_decompress_safe_8     ( const ucl_byte *src, ucl_uint  src_len,
                                        ucl_byte *dst, ucl_uintp dst_len,
                                        ucl_voidp wrkmem );
}

namespace {

const char* VariantName(FUCLDecoder::Variant variant)
{
	switch(variant)
	{
		case FUCLDecoder::Variant::NRV2B: return "UCL2B";
		case FUCLDecoder::Variant::NRV2D: return "UCL2D";
		case FUCLDecoder::Variant::NRV2E: return "UCL2E";
	}
	return "UCL";
}

}

FUCLDecoder::FUCLDecoder(Variant variant) : FBaseDecoder(VariantName(variant)), Kind(variant)
{
}

std::size_t FUCLDecoder::Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const
{
	ucl_uint outLen = Len32(out.size());
	int r = UCL_E_ERROR;
	switch(Kind)
	{
		case Variant::NRV2B: r = ucl_nrv2b_decompress_safe_8(in.data(), Len32(in.size()), out.data(), &outLen, nullptr); break;
		case Variant::NRV2D: r = ucl_nrv2d_decompress_safe_8(in.data(), Len32(in.size()), out.data(), &outLen, nullptr); break;
		case Variant::NRV2E: r = ucl_nrv2e_decompress_safe_8(in.data(), Len32(in.size()), out.data(), &outLen, nullptr); break;
	}
	return (r == UCL_E_OK || r == UCL_E_INPUT_NOT_CONSUMED) ? outLen : 0;
}
