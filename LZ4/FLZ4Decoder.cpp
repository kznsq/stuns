/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include "FLZ4Decoder.h"

#include <optional>

namespace {

// reads an LZ4 length extension (bytes added while 255) onto len; a length beyond
// limit can never be valid
std::optional<std::size_t> ReadLength(std::span<const std::uint8_t> in, std::size_t& ip, std::size_t len, std::size_t limit)
{
	std::uint8_t b;
	do {
		if(ip >= in.size() || len > limit)
			return std::nullopt;
		b = in[ip++];
		len += b;
	} while(b == 255);
	return len;
}

}

std::size_t FLZ4Decoder::Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const
{
	std::size_t ip = 0, op = 0, end = 0;
	bool matched = false, endMatched = false;

	while(ip < in.size())
	{
		const std::uint8_t token = in[ip++];

		// literals
		std::optional<std::size_t> literals = token >> 4;
		if(*literals == 15)
			literals = ReadLength(in, ip, *literals, out.size());
		if(!literals || *literals > in.size() - ip || *literals > out.size() - op)
			break;
		std::copy_n(in.begin() + static_cast<std::ptrdiff_t>(ip), *literals, out.begin() + static_cast<std::ptrdiff_t>(op));
		ip += *literals;
		op += *literals;

		// the block may end here
		end = op;
		endMatched = matched;

		// match
		if(in.size() - ip < 2)
			break;
		const std::size_t offset = in[ip] | (static_cast<std::size_t>(in[ip + 1]) << 8);
		ip += 2;
		if(offset == 0 || offset > op)
			break;
		std::optional<std::size_t> length = token & 15;
		if(*length == 15)
			length = ReadLength(in, ip, *length, out.size());
		if(!length || *length + 4 > out.size() - op)
			break;
		// byte by byte: the source may overlap the destination
		for(std::size_t i = 0; i < *length + 4; i++, op++)
			out[op] = out[op - offset];
		matched = true;
	}

	return endMatched ? end : 0;
}
