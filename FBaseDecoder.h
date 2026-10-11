/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FBASEDECODER_H
#define FBASEDECODER_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>

// Zero bytes kept after the input data: miniLZO 1.08 and the UCL decoders read up to
// 7 bytes past the end of their input before their bounds checks stop them.
inline constexpr std::size_t InputPadding = 64;

// One compression format. Decompress() tries to decode a stream starting at the first
// byte of in, which is always followed by InputPadding readable zero bytes.
class FBaseDecoder
{
public:
	explicit FBaseDecoder(std::string name) : Name(std::move(name)) {}
	virtual ~FBaseDecoder() = default;

	FBaseDecoder(const FBaseDecoder&) = delete;
	FBaseDecoder& operator=(const FBaseDecoder&) = delete;

	const std::string& GetName() const { return Name; }

	// returns the decoded size, or 0 if no stream starts here
	virtual std::size_t Decompress(std::span<const std::uint8_t> in, std::span<std::uint8_t> out) const = 0;

protected:
	// the format libraries take 32-bit lengths
	static std::uint32_t Len32(std::size_t len)
	{
		return static_cast<std::uint32_t>(std::min<std::size_t>(len, std::numeric_limits<std::uint32_t>::max()));
	}

private:
	std::string Name;
};

#endif
