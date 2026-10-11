/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Deflate/FDeflateDecoder.h"
#include "PKWare/FPKWareDecoder.h"
#include "LZO/FLZODecoder.h"
#include "UCL/FUCLDecoder.h"
#include "LZ4/FLZ4Decoder.h"
#include "LZMA/FLZMADecoder.h"
#include "aPLib/FAPLibDecoder.h"
#include "FAnalysis.h"

namespace {

// largest stream a decoder may unpack
constexpr std::size_t MaxOutSize = 256 * 1024 * 1024;

struct Decoder
{
	std::unique_ptr<FBaseDecoder> Format;
	bool Active;
};

struct Options
{
	std::string OutPrefix = "!Out";
	std::filesystem::path InFile;
	// -entropy: show the entropy of every stream found
	bool ShowEntropy = false;
	// -analyze: show entropy, ASCII text, file header and whether the stream looks real
	bool ShowAnalysis = false;
	// -catch: keep only streams that look real (a known file header or enough varied text)
	bool CatchOnly = false;
	// -entropy<max> or -entropy<min>:<max>: keep only streams in this range (bits per byte)
	std::optional<double> MinEntropy;
	std::optional<double> MaxEntropy;
	bool Valid = true;
};

std::vector<Decoder> MakeDecoders()
{
	std::vector<Decoder> decoders;
	decoders.push_back({std::make_unique<FDeflateDecoder>(), true});
	decoders.push_back({std::make_unique<FPKWareDecoder>(), true});
	decoders.push_back({std::make_unique<FLZODecoder>(), true});
	decoders.push_back({std::make_unique<FUCLDecoder>(FUCLDecoder::Variant::NRV2B), true});
	decoders.push_back({std::make_unique<FUCLDecoder>(FUCLDecoder::Variant::NRV2D), true});
	decoders.push_back({std::make_unique<FUCLDecoder>(FUCLDecoder::Variant::NRV2E), true});
	decoders.push_back({std::make_unique<FLZ4Decoder>(), true});
	decoders.push_back({std::make_unique<FLZMADecoder>(), true});
	// many false hits: only on request
	decoders.push_back({std::make_unique<FAPLibDecoder>(), false});
	return decoders;
}

bool EqualNoCase(std::string_view a, std::string_view b)
{
	return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
		return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
	});
}

bool StartsWithNoCase(std::string_view s, std::string_view prefix)
{
	return s.size() >= prefix.size() && EqualNoCase(s.substr(0, prefix.size()), prefix);
}

void Usage()
{
	std::cout <<
		"STUNS - STupid UNcompreSsor v1.0\n"
		"(c) Andrii Frolov aka kznsq, 2004-2026\n"
		"falinc@ukr.net\n"
		"Usage: stuns [options] <filename>\n\n"
		"Options are: -deflate[-] Deflate format\n"
		"             -pkware[-]  PKWare Compression Library format\n"
		"             -lzo[-]     LZO compression library format\n"
		"             -ucl2b[-]   UCL nrv2b compression library format\n"
		"             -ucl2d[-]   UCL nrv2d compression library format\n"
		"             -ucl2e[-]   UCL nrv2e compression library format\n"
		"             -lz4[-]     LZ4 block format\n"
		"             -lzma[-]    LZMA raw stream (lc=3, lp=0, pb=2)\n"
		"             -aplib[-]   aPLib format (off by default: many false hits)\n"
		"             -Out<dir>   prefix for out directory (default '!Out')\n"
		"             -analyze    show entropy, ASCII text and file header of every stream\n"
		"             -catch      keep only streams that look real: a known file header\n"
		"                         or enough varied ASCII text\n"
		"             -entropy    show the entropy of every stream found (bits per byte)\n"
		"             -entropy<max>, -entropy<min>:<max>, -entropy<min>:\n"
		"                         keep only streams with entropy in this range\n"
		"Note: adding '-' after a switch turns it off\n";
}

// a whole decimal number such as 7 or 6.5
bool ParseNumber(std::string_view text, std::optional<double>& value)
{
	const std::string str(text);
	char* end = nullptr;
	const double number = std::strtod(str.c_str(), &end);
	if(str.empty() || end != str.c_str() + str.size())
		return false;
	value = number;
	return true;
}

// -<format> turns a decoder on, -<format>- off, -Out<prefix> sets the output prefix;
// anything else is the input file name
Options ParseCommandLine(std::span<char*> args, std::vector<Decoder>& decoders)
{
	Options options;
	for(std::string_view arg : args)
	{
		if(!arg.starts_with('-'))
		{
			options.InFile = arg;
			continue;
		}
		if(arg.size() > 4 && StartsWithNoCase(arg, "-Out"))
		{
			options.OutPrefix = arg.substr(4);
			continue;
		}
		if(EqualNoCase(arg, "-analyze"))
		{
			options.ShowAnalysis = true;
			continue;
		}
		if(EqualNoCase(arg, "-catch"))
		{
			options.ShowAnalysis = options.CatchOnly = true;
			continue;
		}
		if(StartsWithNoCase(arg, "-entropy"))
		{
			options.ShowEntropy = true;
			const std::string_view range = arg.substr(8);
			const std::size_t colon = range.find(':');
			if(colon == std::string_view::npos)
			{
				if(!range.empty())
					options.Valid &= ParseNumber(range, options.MaxEntropy);
			}
			else
			{
				if(colon > 0)
					options.Valid &= ParseNumber(range.substr(0, colon), options.MinEntropy);
				if(colon + 1 < range.size())
					options.Valid &= ParseNumber(range.substr(colon + 1), options.MaxEntropy);
			}
			continue;
		}
		const std::string_view name = arg.substr(1);
		for(auto& decoder : decoders)
		{
			const std::string& format = decoder.Format->GetName();
			if(EqualNoCase(name, format))
				decoder.Active = true;
			else if(name.size() == format.size() + 1 && name.back() == '-' && StartsWithNoCase(name, format))
				decoder.Active = false;
		}
	}
	return options;
}

// the whole file followed by InputPadding zero bytes
std::optional<std::vector<std::uint8_t>> ReadInput(const std::filesystem::path& path)
{
	std::error_code ec;
	const auto size = std::filesystem::file_size(path, ec);
	if(ec || size > std::vector<std::uint8_t>().max_size() - InputPadding)
		return std::nullopt;
	std::ifstream file(path, std::ios::binary);
	if(!file)
		return std::nullopt;
	std::vector<std::uint8_t> data(static_cast<std::size_t>(size) + InputPadding);
	if(!file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size)))
		return std::nullopt;
	return data;
}

// the analysis part of a result line
std::string Describe(const Options& options, const FAnalysis& a)
{
	if(!options.ShowAnalysis)
		return options.ShowEntropy ? std::format(", entropy {:.2f}", a.Entropy) : std::string();
	std::string text = std::format(", entropy {:.2f}, text {:.0f}%", a.Entropy, a.TextShare * 100);
	if(a.LongestText)
		text += std::format(" (longest {})", a.LongestText);
	if(!a.Header.empty())
		text += ", header: " + a.Header;
	if(const std::string catchName = a.Catch(); !catchName.empty())
		text += " <== catch: " + catchName;
	return text;
}

// saves one stream as <prefix><format>/<offset in hex>
void SaveFile(const Options& options, const FBaseDecoder& format, std::size_t offset, std::span<const std::uint8_t> data, const std::string& note)
{
	const std::filesystem::path dir = options.OutPrefix + format.GetName();
	const std::filesystem::path name = dir / std::format("{:04x}", offset);
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);
	std::ofstream file(name, std::ios::binary);
	if(!file || !file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size())) || !file.flush())
	{
		std::cout << std::format("{}: Can't write output file: {}\n", format.GetName(), name.string());
		return;
	}
	std::cout << std::format("{}: Succeeded uncompressing {} bytes at: {} (0x{:x}){}\n", format.GetName(), data.size(), offset, offset, note);
}

}

int main(int argc, char** argv)
{
	auto decoders = MakeDecoders();
	const Options options = ParseCommandLine(std::span<char*>(argv, static_cast<std::size_t>(argc)).subspan(1), decoders);

	std::error_code ec;
	if(!options.Valid || options.InFile.empty() || !std::filesystem::is_regular_file(options.InFile, ec))
	{
		Usage();
		return -1;
	}

	const auto input = ReadInput(options.InFile);
	if(!input)
	{
		std::cout << "Can't load input file: " << options.InFile.string() << "\n";
		return -1;
	}

	std::vector<std::uint8_t> out(MaxOutSize);
	const auto data = std::span<const std::uint8_t>(*input).first(input->size() - InputPadding);

	// try every active decoder at every offset
	std::size_t skipped = 0;
	for(std::size_t pos = 0; pos < data.size(); pos++)
	{
		for(const auto& [format, active] : decoders)
		{
			if(!active)
				continue;
			const std::size_t size = format->Decompress(data.subspan(pos), out);
			if(!size)
				continue;
			const auto stream = std::span<const std::uint8_t>(out).first(size);
			if(!options.ShowEntropy && !options.ShowAnalysis)
			{
				SaveFile(options, *format, pos, stream, {});
				continue;
			}
			const FAnalysis analysis = Analyze(stream);
			if((options.MinEntropy && analysis.Entropy < *options.MinEntropy)
				|| (options.MaxEntropy && analysis.Entropy > *options.MaxEntropy)
				|| (options.CatchOnly && analysis.Catch().empty()))
			{
				skipped++;
				continue;
			}
			SaveFile(options, *format, pos, stream, Describe(options, analysis));
		}
	}

	if(options.CatchOnly || options.MinEntropy || options.MaxEntropy)
		std::cout << std::format("{} streams skipped by the filters\n", skipped);

	return 0;
}
