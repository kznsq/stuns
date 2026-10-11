/************************************************************************************
 STUNS v1.0
 (c) Andrii Frolov aka kznsq, 2004-2026
 falinc@ukr.net
************************************************************************************/

#ifndef FANALYSIS_H
#define FANALYSIS_H

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

// What a decoded stream looks like, to tell real data from the junk that decoders
// produce from bytes that only look like a stream.
//
// Measured on random data and executables, junk is mostly short, or long runs of one
// byte or one short pattern (low entropy even when it is printable), while real text
// sits around 4..5 bits per byte.
//
// Not caught: decoders run over a region of text (string tables in executables) can
// splice pieces of it into long "word salad" that has the same statistics as real text.
struct FAnalysis
{
	std::size_t Size = 0;
	double Entropy = 0;             // order-0 Shannon entropy, bits per byte, 0..8
	double TextShare = 0;           // part of the bytes inside printable ASCII runs of 4+ characters
	std::size_t LongestText = 0;    // longest such run
	std::string Header;             // file format recognized from the first bytes, empty if none

	// minimum size and entropy for mostly-text output to count as a catch
	static constexpr std::size_t MinTextSize = 64;
	static constexpr double MinTextShare = 0.75;
	static constexpr double MinTextEntropy = 3.0;

	// likely real data: a known file header, or enough varied text; empty if not
	std::string Catch() const;

	// at least MinTextSize bytes of mostly varied text
	bool IsText() const;
};

FAnalysis Analyze(std::span<const std::uint8_t> data);

#endif
