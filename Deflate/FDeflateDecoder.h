/************************************************************************************
 STUNS v0.1
 (c) Andrii Frolov aka kznsq, 2004
 falinc@ukr.net
************************************************************************************/

#ifndef FDEFLATEDECODER_H
#define FDEFLATEDECODER_H

#include "..\\FBaseDecoder.h"

class FDeflateDecoder : public FBaseDecoder {
public:
	bool Decompress(void);

	FDeflateDecoder(void) : FBaseDecoder()
	{
		strcpy(ClassName, "Deflate");
	}

	~FDeflateDecoder(void)
	{
	}
};

#endif
