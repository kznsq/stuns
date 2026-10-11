/************************************************************************************
 STUNS v0.1
 (c) Andrii Frolov aka kznsq, 2004
 falinc@ukr.net
************************************************************************************/

#ifndef FUCL2BDECODER_H
#define FUCL2BDECODER_H

#include "..\\FBaseDecoder.h"

class FUCL2BDecoder : public FBaseDecoder {
public:
	bool Decompress(void);

	FUCL2BDecoder(void) : FBaseDecoder()
	{
		strcpy(ClassName, "UCL2B");
	}

	~FUCL2BDecoder(void)
	{
	}
};

#endif
