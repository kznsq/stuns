/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include "FAnalysis.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace {

struct Signature
{
	std::string_view Name;
	std::size_t Offset;
	std::string_view Magic;
};

using namespace std::string_view_literals;

// signatures of 3 bytes or more; shorter ones (BM, MZ) are checked with their headers below
constexpr std::array Signatures{
	Signature{"PNG image", 0, "\x89PNG\r\n\x1a\n"sv},
	Signature{"GIF image", 0, "GIF87a"sv},
	Signature{"GIF image", 0, "GIF89a"sv},
	Signature{"JPEG image", 0, "\xFF\xD8\xFF"sv},
	Signature{"TIFF image", 0, "II*\0"sv},
	Signature{"TIFF image", 0, "MM\0*"sv},
	Signature{"Photoshop image", 0, "8BPS"sv},
	Signature{"DDS texture", 0, "DDS "sv},
	Signature{"IFF file", 0, "FORM"sv},
	Signature{"MIDI music", 0, "MThd"sv},
	Signature{"Ogg file", 0, "OggS"sv},
	Signature{"FLAC audio", 0, "fLaC"sv},
	Signature{"MP3 with ID3 tag", 0, "ID3"sv},
	Signature{"Creative voice file", 0, "Creative Voice File\x1a"sv},
	Signature{"FastTracker module", 0, "Extended Module: "sv},
	Signature{"Impulse Tracker module", 0, "IMPM"sv},
	Signature{"ScreamTracker module", 44, "SCRM"sv},
	Signature{"ProTracker module", 1080, "M.K."sv},
	Signature{"ProTracker module", 1080, "M!K!"sv},
	Signature{"ProTracker module", 1080, "4CHN"sv},
	Signature{"ProTracker module", 1080, "6CHN"sv},
	Signature{"ProTracker module", 1080, "8CHN"sv},
	Signature{"ZIP archive", 0, "PK\x03\x04"sv},
	Signature{"gzip data", 0, "\x1F\x8B\x08"sv},
	Signature{"7-Zip archive", 0, "7z\xBC\xAF\x27\x1C"sv},
	Signature{"RAR archive", 0, "Rar!\x1A\x07"sv},
	Signature{"PDF document", 0, "%PDF-"sv},
	Signature{"XML document", 0, "<?xml"sv},
	Signature{"ELF executable", 0, "\x7F" "ELF"sv},
	Signature{"Mach-O executable", 0, "\xFE\xED\xFA\xCE"sv},
	Signature{"Mach-O executable", 0, "\xFE\xED\xFA\xCF"sv},
	Signature{"Mach-O executable", 0, "\xCE\xFA\xED\xFE"sv},
	Signature{"Mach-O executable", 0, "\xCF\xFA\xED\xFE"sv},
	Signature{"Java class", 0, "\xCA\xFE\xBA\xBE"sv},
	Signature{"WebAssembly module", 0, "\0asm"sv},
	Signature{"SQLite database", 0, "SQLite format 3\0"sv},
	Signature{"Lua bytecode", 0, "\x1BLua"sv},
};

std::uint32_t Read16(std::span<const std::uint8_t> d, std::size_t at)
{
	return d[at] | (static_cast<std::uint32_t>(d[at + 1]) << 8);
}

std::uint32_t Read32(std::span<const std::uint8_t> d, std::size_t at)
{
	return Read16(d, at) | (Read16(d, at + 2) << 16);
}

bool Matches(std::span<const std::uint8_t> d, std::size_t at, std::string_view magic)
{
	return d.size() >= at + magic.size()
		&& std::equal(magic.begin(), magic.end(), d.begin() + static_cast<std::ptrdiff_t>(at),
			[](char m, std::uint8_t b) { return static_cast<std::uint8_t>(m) == b; });
}

bool IsPrintable4(std::span<const std::uint8_t> d, std::size_t at)
{
	return d.size() >= at + 4 && std::all_of(d.begin() + static_cast<std::ptrdiff_t>(at), d.begin() + static_cast<std::ptrdiff_t>(at + 4),
		[](std::uint8_t b) { return b >= 0x20 && b < 0x7F; });
}

std::string DetectHeader(std::span<const std::uint8_t> d)
{
	// RIFF: the form type names the format (WAVE, AVI , WEBP, ...)
	if(Matches(d, 0, "RIFF") && IsPrintable4(d, 8))
		return "RIFF " + std::string(d.begin() + 8, d.begin() + 12) + " file";

	for(const auto& s : Signatures)
		if(Matches(d, s.Offset, s.Magic))
			return std::string(s.Name);

	// BMP: pixel data offset inside the file and a known info header size
	if(Matches(d, 0, "BM") && d.size() >= 18)
	{
		const std::uint32_t pixels = Read32(d, 10), info = Read32(d, 14);
		if(pixels >= 26 && pixels < d.size() && (info == 12 || info == 40 || info == 52 || info == 56 || info == 108 || info == 124))
			return "BMP image";
	}

	// MZ: Windows PE when e_lfanew points at "PE\0\0", else a consistent DOS header
	if(Matches(d, 0, "MZ") && d.size() >= 0x40)
	{
		const std::uint32_t lfanew = Read32(d, 0x3C);
		if(lfanew >= 0x40 && lfanew <= d.size() - 4 && Matches(d, lfanew, std::string_view("PE\0\0", 4)))
			return "Windows executable";
		const std::uint32_t lastPage = Read16(d, 2), pages = Read16(d, 4), headerParagraphs = Read16(d, 8);
		if(lastPage < 512 && pages > 0 && headerParagraphs >= 2 && headerParagraphs * 16u < pages * 512u
			&& (pages - 1) * 512u + lastPage <= d.size() + 512)
			return "DOS executable";
	}
	return {};
}

bool IsTextByte(std::uint8_t b)
{
	return (b >= 0x20 && b < 0x7F) || b == '\t' || b == '\r' || b == '\n';
}

}

FAnalysis Analyze(std::span<const std::uint8_t> data)
{
	FAnalysis a;
	a.Size = data.size();
	if(data.empty())
		return a;

	std::array<std::size_t, 256> counts{};
	std::size_t run = 0, inRuns = 0;
	for(const std::uint8_t b : data)
	{
		counts[b]++;
		if(IsTextByte(b))
		{
			run++;
			continue;
		}
		if(run >= 4)
			inRuns += run;
		a.LongestText = std::max(a.LongestText, run);
		run = 0;
	}
	if(run >= 4)
		inRuns += run;
	a.LongestText = std::max(a.LongestText, run);
	if(a.LongestText < 4)
		a.LongestText = 0;

	for(const std::size_t count : counts)
	{
		if(count)
		{
			const double p = static_cast<double>(count) / static_cast<double>(data.size());
			a.Entropy -= p * std::log2(p);
		}
	}
	a.TextShare = static_cast<double>(inRuns) / static_cast<double>(data.size());
	a.Header = DetectHeader(data);
	return a;
}

bool FAnalysis::IsText() const
{
	return Size >= MinTextSize && TextShare >= MinTextShare && Entropy >= MinTextEntropy;
}

std::string FAnalysis::Catch() const
{
	if(!Header.empty())
		return Header;
	if(IsText())
		return "text";
	return {};
}
