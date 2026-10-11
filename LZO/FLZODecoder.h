/************************************************************************************
 STUNS v0.1
 (c) Andrii Frolov aka kznsq, 2004
 falinc@ukr.net
************************************************************************************/

#ifndef FLZODECODER_H
#define FLZODECODER_H

#include "..\\FBaseDecoder.h"

class FLZODecoder : public FBaseDecoder {
public:
	bool Decompress(void);

	FLZODecoder(void) : FBaseDecoder()
	{
		strcpy(ClassName, "LZO");
	}

	~FLZODecoder(void)
	{
	}
};

#endif
