/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include "FPKWareDecoder.h"

#include <algorithm>

extern "C" {
#include "blast.h"
}

namespace {

// state shared with the blast() input and output callbacks
struct BlastIo
{
	std::span<const std::uint8_t> In;
	std::span<std::uint8_t> Out;
	std::size_t OutPos = 0;
	bool InGiven = false;
};

unsigned BlastIn(void* how, unsigned char** buf)
{
	auto& io = *static_cast<BlastIo*>(how);
	// the whole input in one piece, then end of input
	if(io.InGiven)
		return 0;
	io.InGiven = true;
	// blast() never writes to the input buffer
	*buf = const_cast<unsigned char*>(io.In.data());
	return static_cast<unsigned>(std::min<std::size_t>(io.In.size(), 0xFFFFFFFFu));
}

int BlastOut(void* how, unsigned char* buf, unsigned len)
{
	auto& io = *static_cast<BlastIo*>(how);
	// output does not fit: blast() stops with an error
	if(len > io.Out.size() - io.OutPos)
		return 1;
	std::copy_n(buf, len, io.Out.begin() + static_cast<std::ptrdiff_t>(io.OutPos));
	io.OutPos += len;
	return 0;
}

}

std::size_t FPKWareDecoder::Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const
{
	BlastIo io{in, out};
	const int ret = blast(BlastIn, &io, BlastOut, &io);
	return ret == 0 ? io.OutPos : 0;
}
