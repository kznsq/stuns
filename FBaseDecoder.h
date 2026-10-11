/************************************************************************************
 STUNS v0.1
 (c) Andrii Frolov aka kznsq, 2004
 falinc@ukr.net
************************************************************************************/

#ifndef FBASEDECODER_H
#define FBASEDECODER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

class FBaseDecoder {
public:
	enum {
		MAX_OUT_SIZE = 256*1024*1024,
		// zero bytes after the input: the UCL decoders may read 1 byte past the end before their bounds check
		IN_PADDING = 64
	};

	static std::string OutDir;
	char ClassName[64];

	static unsigned char* InData;
	static unsigned char* OutData;
	static unsigned long InLen, InPos;
	static unsigned long OutLen, OutPos;

	bool Initialize(const char* InFileName);
	bool Release(void);
	virtual bool Decompress(void) = 0;
	bool SaveFile(void);

	FBaseDecoder(void)
	{
		ClassName[0] = 0;
	}

	virtual ~FBaseDecoder(void)
	{
		Release();
	}
};

#endif
