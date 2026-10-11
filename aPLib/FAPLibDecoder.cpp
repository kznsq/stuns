/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include "FAPLibDecoder.h"


namespace {

// aPLib stream reader: tag bits are read most significant first, 8 per byte, the tag
// bytes interleaved with data bytes; any read past the input marks the stream bad
class Reader
{
public:
	explicit Reader(std::span<const std::uint8_t> in) : In(in) {}

	bool Bad() const { return Failed; }

	unsigned Byte()
	{
		if(Pos >= In.size())
		{
			Failed = true;
			return 0;
		}
		return In[Pos++];
	}

	unsigned Bit()
	{
		if(!Bits)
		{
			Tag = Byte();
			Bits = 8;
		}
		Bits--;
		return (Tag >> Bits) & 1;
	}

	// Elias gamma: a leading 1, then (bit, continue flag) pairs
	std::size_t Gamma()
	{
		std::size_t v = 1;
		do {
			if(v > 0x7FFFFFFF)
			{
				Failed = true;
				return 0;
			}
			v = (v << 1) + Bit();
		} while(Bit() && !Failed);
		return v;
	}

private:
	std::span<const std::uint8_t> In;
	std::size_t Pos = 0;
	unsigned Tag = 0;
	unsigned Bits = 0;
	bool Failed = false;
};

// copies len bytes from offset back, byte by byte: the source may overlap the destination
bool Copy(std::span<std::uint8_t> out, std::size_t& op, std::size_t offset, std::size_t len)
{
	if(offset == 0 || offset > op || len > out.size() - op)
		return false;
	for(std::size_t i = 0; i < len; i++, op++)
		out[op] = out[op - offset];
	return true;
}

}

std::size_t FAPLibDecoder::Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const
{
	if(in.empty() || out.empty())
		return 0;

	Reader r(in);
	std::size_t op = 0;
	std::size_t lastOffset = 0;
	bool lastWasMatch = false;

	// the first byte is always a literal
	out[op++] = static_cast<std::uint8_t>(r.Byte());

	while(!r.Bad())
	{
		if(!r.Bit())
		{
			// 0: literal byte
			if(op >= out.size())
				return 0;
			out[op++] = static_cast<std::uint8_t>(r.Byte());
			lastWasMatch = false;
		}
		else if(!r.Bit())
		{
			// 10: match with a gamma-coded offset, or the last offset again
			std::size_t offset = r.Gamma();
			std::size_t len;
			if(!lastWasMatch && offset == 2)
			{
				offset = lastOffset;
				len = r.Gamma();
			}
			else
			{
				offset -= lastWasMatch ? 2 : 3;
				if(offset > 0xFFFFFF)
					return 0;
				offset = (offset << 8) + r.Byte();
				len = r.Gamma();
				if(offset >= 32000) len++;
				if(offset >= 1280) len++;
				if(offset < 128) len += 2;
				lastOffset = offset;
			}
			if(r.Bad() || !Copy(out, op, offset, len))
				return 0;
			lastWasMatch = true;
		}
		else if(!r.Bit())
		{
			// 110: short match, 7-bit offset and length 2 or 3; offset 0 ends the stream
			const unsigned b = r.Byte();
			if(r.Bad())
				return 0;
			if(!(b >> 1))
				return op;
			if(!Copy(out, op, b >> 1, 2 + (b & 1)))
				return 0;
			lastOffset = b >> 1;
			lastWasMatch = true;
		}
		else
		{
			// 111: one byte from a 4-bit offset; offset 0 writes a zero
			std::size_t offset = 0;
			for(int i = 0; i < 4; i++)
				offset = (offset << 1) + r.Bit();
			if(r.Bad() || op >= out.size() || offset > op)
				return 0;
			out[op] = offset ? out[op - offset] : 0;
			op++;
			lastWasMatch = false;
		}
	}
	return 0;
}
