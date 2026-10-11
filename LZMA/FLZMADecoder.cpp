/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include "FLZMADecoder.h"

#include <array>
#include <memory>

namespace {

constexpr unsigned LC = 3;
constexpr unsigned LP = 0;
constexpr unsigned PB = 2;

constexpr unsigned NumStates = 12;
constexpr unsigned PosStatesMax = 1 << 4;
constexpr unsigned MatchMinLen = 2;
constexpr unsigned EndPosModel = 14;
constexpr unsigned FullDistances = 128;
constexpr unsigned AlignBits = 4;
constexpr unsigned LenToPosStates = 4;

using Prob = std::uint16_t;
constexpr Prob ProbInit = 1 << 10;     // probability 0.5 with 11-bit precision

template <std::size_t N>
using Probs = std::array<Prob, N>;

template <std::size_t N>
void Init(Probs<N>& probs)
{
	probs.fill(ProbInit);
}

template <std::size_t N, std::size_t M>
void Init(std::array<Probs<N>, M>& table)
{
	for(auto& probs : table)
		probs.fill(ProbInit);
}

struct LenModel
{
	LenModel()
	{
		Init(Low);
		Init(Mid);
		Init(High);
	}

	Prob Choice = ProbInit;
	Prob Choice2 = ProbInit;
	std::array<Probs<1 << 3>, PosStatesMax> Low;
	std::array<Probs<1 << 3>, PosStatesMax> Mid;
	Probs<1 << 8> High;
};

// every adaptive probability of the decoder, all starting at ProbInit
struct Model
{
	Model()
	{
		Init(Literal);
		Init(PosSlot);
		Init(Pos);
		Init(Align);
		Init(IsMatch);
		Init(IsRep);
		Init(IsRepG0);
		Init(IsRepG1);
		Init(IsRepG2);
		Init(IsRep0Long);
	}

	Probs<0x300 << (LC + LP)> Literal;
	std::array<Probs<1 << 6>, LenToPosStates> PosSlot;
	Probs<1 + FullDistances - EndPosModel> Pos;
	Probs<1 << AlignBits> Align;
	Probs<NumStates << 4> IsMatch;
	Probs<NumStates> IsRep;
	Probs<NumStates> IsRepG0;
	Probs<NumStates> IsRepG1;
	Probs<NumStates> IsRepG2;
	Probs<NumStates << 4> IsRep0Long;
	LenModel Len;
	LenModel RepLen;
};

class RangeDecoder
{
public:
	// in starts with the 5 range coder init bytes
	explicit RangeDecoder(std::span<const std::uint8_t> in) : In(in)
	{
		for(int i = 0; i < 4; i++)
			Code = (Code << 8) | In[Pos++];
		// a real stream never starts with code 0 (even for all-zero data); inside runs of
		// zero bytes it would decode zeros up to the output limit at every offset
		Corrupted = Code == Range || Code == 0;
	}

	bool Over() const { return InputOver; }
	bool Bad() const { return Corrupted; }
	bool FinishedOk() const { return Code == 0 && !InputOver; }
	void MarkBad() { Corrupted = true; }

	unsigned Bit(Prob& p)
	{
		const std::uint32_t bound = (Range >> 11) * p;
		unsigned bit;
		if(Code < bound)
		{
			p = static_cast<Prob>(p + (((1 << 11) - p) >> 5));
			Range = bound;
			bit = 0;
		}
		else
		{
			p = static_cast<Prob>(p - (p >> 5));
			Code -= bound;
			Range -= bound;
			bit = 1;
		}
		Normalize();
		return bit;
	}

	std::uint32_t Direct(unsigned bits)
	{
		std::uint32_t res = 0;
		do {
			Range >>= 1;
			Code -= Range;
			const std::uint32_t t = 0 - (Code >> 31);
			Code += Range & t;
			if(Code == Range)
				Corrupted = true;
			Normalize();
			res = (res << 1) + (t + 1);
		} while(--bits);
		return res;
	}

	template <std::size_t N>
	std::uint32_t Tree(Probs<N>& probs, unsigned bits)
	{
		std::uint32_t m = 1;
		for(unsigned i = 0; i < bits; i++)
			m = (m << 1) + Bit(probs[m]);
		return m - (1u << bits);
	}

	std::uint32_t TreeReverse(std::span<Prob> probs, unsigned bits)
	{
		std::uint32_t m = 1, symbol = 0;
		for(unsigned i = 0; i < bits; i++)
		{
			const unsigned bit = Bit(probs[m]);
			m = (m << 1) + bit;
			symbol |= bit << i;
		}
		return symbol;
	}

private:
	void Normalize()
	{
		if(Range < (1u << 24))
		{
			Range <<= 8;
			Code = (Code << 8) | NextByte();
		}
	}

	std::uint8_t NextByte()
	{
		if(Pos >= In.size())
		{
			InputOver = true;
			return 0;
		}
		return In[Pos++];
	}

	std::span<const std::uint8_t> In;
	std::size_t Pos = 1;
	std::uint32_t Range = 0xFFFFFFFFu;
	std::uint32_t Code = 0;
	bool InputOver = false;
	bool Corrupted = false;
};

std::uint32_t DecodeLen(RangeDecoder& rc, LenModel& m, unsigned posState)
{
	if(!rc.Bit(m.Choice))
		return rc.Tree(m.Low[posState], 3);
	if(!rc.Bit(m.Choice2))
		return 8 + rc.Tree(m.Mid[posState], 3);
	return 16 + rc.Tree(m.High, 8);
}

std::uint32_t DecodeDistance(RangeDecoder& rc, Model& m, std::uint32_t len)
{
	const unsigned lenState = len < LenToPosStates - 1 ? len : LenToPosStates - 1;
	const std::uint32_t slot = rc.Tree(m.PosSlot[lenState], 6);
	if(slot < 4)
		return slot;
	const unsigned direct = (slot >> 1) - 1;
	std::uint32_t dist = (2 | (slot & 1)) << direct;
	if(slot < EndPosModel)
		return dist + rc.TreeReverse(std::span<Prob>(m.Pos).subspan(dist - slot), direct);
	dist += rc.Direct(direct - AlignBits) << AlignBits;
	return dist + rc.TreeReverse(m.Align, AlignBits);
}

}

std::size_t FLZMADecoder::Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const
{
	// the range coder always starts with a zero byte
	if(in.size() < 5 || in[0] != 0)
		return 0;

	RangeDecoder rc(in);
	if(rc.Bad())
		return 0;

	const auto model = std::make_unique<Model>();
	Model& m = *model;

	std::size_t pos = 0;
	unsigned state = 0;
	std::uint32_t rep0 = 0, rep1 = 0, rep2 = 0, rep3 = 0;
	bool endMarker = false;

	while(!rc.Over() && !rc.Bad())
	{
		const unsigned posState = pos & ((1 << PB) - 1);

		if(!rc.Bit(m.IsMatch[(state << 4) + posState]))
		{
			// literal
			if(pos >= out.size())
			{
				rc.MarkBad();
				break;
			}
			const unsigned prev = pos ? out[pos - 1] : 0;
			auto probs = std::span<Prob>(m.Literal).subspan(0x300 * ((((pos & ((1 << LP) - 1)) << LC) + (prev >> (8 - LC)))), 0x300);
			unsigned symbol = 1;
			if(state >= 7)
			{
				unsigned matchByte = out[pos - rep0 - 1];
				do {
					const unsigned matchBit = (matchByte >> 7) & 1;
					matchByte <<= 1;
					const unsigned bit = rc.Bit(probs[((1 + matchBit) << 8) + symbol]);
					symbol = (symbol << 1) | bit;
					if(matchBit != bit)
						break;
				} while(symbol < 0x100);
			}
			while(symbol < 0x100)
				symbol = (symbol << 1) | rc.Bit(probs[symbol]);
			out[pos++] = static_cast<std::uint8_t>(symbol - 0x100);
			state = state < 4 ? 0 : (state < 10 ? state - 3 : state - 6);
			continue;
		}

		std::uint32_t len;
		if(rc.Bit(m.IsRep[state]))
		{
			// repeated distance
			if(pos == 0)
			{
				rc.MarkBad();
				break;
			}
			if(!rc.Bit(m.IsRepG0[state]))
			{
				if(!rc.Bit(m.IsRep0Long[(state << 4) + posState]))
				{
					// short rep: one byte
					if(pos >= out.size())
					{
						rc.MarkBad();
						break;
					}
					state = state < 7 ? 9 : 11;
					out[pos] = out[pos - rep0 - 1];
					pos++;
					continue;
				}
			}
			else
			{
				std::uint32_t dist;
				if(!rc.Bit(m.IsRepG1[state]))
					dist = rep1;
				else
				{
					if(!rc.Bit(m.IsRepG2[state]))
						dist = rep2;
					else
					{
						dist = rep3;
						rep3 = rep2;
					}
					rep2 = rep1;
				}
				rep1 = rep0;
				rep0 = dist;
			}
			len = DecodeLen(rc, m.RepLen, posState);
			state = state < 7 ? 8 : 11;
		}
		else
		{
			// new distance
			rep3 = rep2;
			rep2 = rep1;
			rep1 = rep0;
			len = DecodeLen(rc, m.Len, posState);
			state = state < 7 ? 7 : 10;
			rep0 = DecodeDistance(rc, m, len);
			if(rep0 == 0xFFFFFFFFu)
			{
				// end marker: the range coder must end cleanly
				endMarker = rc.FinishedOk();
				break;
			}
			if(rep0 >= pos)
			{
				rc.MarkBad();
				break;
			}
		}

		// copy the match byte by byte: the source may overlap the destination
		len += MatchMinLen;
		if(len > out.size() - pos)
		{
			rc.MarkBad();
			break;
		}
		for(std::uint32_t i = 0; i < len; i++, pos++)
			out[pos] = out[pos - rep0 - 1];
	}

	return endMarker ? pos : 0;
}
