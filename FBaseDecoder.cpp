/************************************************************************************
 STUNS v0.1
 (c) Andrii Frolov aka kznsq, 2004
 falinc@ukr.net
************************************************************************************/

//-----------------------------------------------------------------------------------
#include "FBaseDecoder.h"

#ifdef _WIN32
#include <direct.h>
#define MakeDir(s) _mkdir(s)
#else
#include <sys/stat.h>
#define MakeDir(s) mkdir(s, 0777)
#endif
//-----------------------------------------------------------------------------------
unsigned char* FBaseDecoder::InData = NULL;
unsigned char* FBaseDecoder::OutData = NULL;
unsigned long FBaseDecoder::InLen = 0, FBaseDecoder::InPos = 0;
unsigned long FBaseDecoder::OutLen = MAX_OUT_SIZE, FBaseDecoder::OutPos = 0;
std::string FBaseDecoder::OutDir;
//-----------------------------------------------------------------------------------
bool FBaseDecoder::Initialize(const char* InFileName)
{
	if(InData) return true;

	// - input file, read whole into memory and followed by IN_PADDING zero bytes
	FILE* in = fopen(InFileName, "rb");
	if(!in) return false;

	// decoder lengths are 32-bit
	const size_t MaxLen = 0xFFFFFFFFUL - IN_PADDING;
	size_t Size = 0, Len = 0, Got;
	bool ok = true;
	for(;;)
	{
		if(Len + IN_PADDING >= Size)
		{
			if(!Size)
				Size = 1024 * 1024;
			else
				Size = (Size > (MaxLen + IN_PADDING) / 2) ? MaxLen + IN_PADDING : Size * 2;
			if(Len + IN_PADDING >= Size)
			{
				// file too large
				ok = false;
				break;
			}
			unsigned char* Data = (unsigned char*)realloc(InData, Size);
			if(!Data)
			{
				ok = false;
				break;
			}
			InData = Data;
		}
		Got = fread(InData + Len, 1, Size - IN_PADDING - Len, in);
		Len += Got;
		if(!Got)
		{
			ok = !ferror(in);
			break;
		}
	}
	fclose(in);

	// - output buffer
	if(ok)
	{
		OutData = (unsigned char*)malloc(MAX_OUT_SIZE);
		ok = OutData != NULL;
	}

	if(!ok)
	{
		free(InData);
		InData = NULL;
		return false;
	}

	memset(InData + Len, 0, IN_PADDING);
	InLen = (unsigned long)Len;
	return true;
}
//-----------------------------------------------------------------------------------
bool FBaseDecoder::Release(void)
{
	if(!InData) return true;

	free(InData);
	free(OutData);
	InData = OutData = NULL;
	InLen = 0;

	return true;
}
//-----------------------------------------------------------------------------------
bool FBaseDecoder::SaveFile(void)
{
	if(OutDir.empty())
		OutDir = "!Out";

	std::string Dir = OutDir + ClassName;
	MakeDir(Dir.c_str());

	char Name[32];
	snprintf(Name, sizeof(Name), "/%04lx", InPos);
	std::string OutFileName = Dir + Name;
	FILE* out = fopen(OutFileName.c_str(), "wb");
	if(!out)
	{
		printf("%s: Can't create output file: %s\n", ClassName, OutFileName.c_str());
		return false;
	}
	bool ok = fwrite(OutData, 1, OutLen, out) == OutLen;
	ok = fclose(out) == 0 && ok;
	if(!ok)
	{
		printf("%s: Can't write output file: %s\n", ClassName, OutFileName.c_str());
		return false;
	}
	printf("%s: Succeeded uncompressing %lu bytes at: %lu (0x%lx)\n", ClassName, OutLen, InPos, InPos);
	return true;
}
//-----------------------------------------------------------------------------------
