/*
 * crc32.c
 *
 *  Created on: Oct 20, 2015
 *      Author: spark
 */
#include <stdint.h>
#include "crc32_table.h"
//int endianTest();
// TODO: change name to endian convert function.
static uint32_t byteTo32bits(uint8_t *pDspCode)
{
	uint32_t i, temp;

	temp = *pDspCode++;
	for (i = 0; i < 3; i++)
	{
		temp <<= 8;
		temp |= *pDspCode++;
	}
	return (temp);
}

void crc32_getCrc(const unsigned char *pSrc, unsigned int srcLength,
		unsigned int *pCrcVal)
{
	unsigned int crcState = 0xFFFFFFFF;
	unsigned int offset = 0;
	unsigned char byte = 0;

	for (offset = 0; offset < srcLength; offset++)
	{
		byte = (crcState >> 24) ^ pSrc[offset];
		crcState = crc32Table[byte] ^ (crcState << 8);
	}

	*pCrcVal = (~crcState);
}

static void crc32_initCrc(unsigned int *pCrcVal)
{
	*pCrcVal = 0xffffffff;
}

static void crc32_updateCrc(unsigned int *pCrcVal, unsigned char *pCharVal)
{
	unsigned char byte = 0;
	byte = (((*pCrcVal) >> 24) ^ (*pCharVal));
	*pCrcVal = ((*pCrcVal) << 8) ^ crc32Table[byte];
}

static void crc32_finalCrc(unsigned int *pCrcVal)
{
	*pCrcVal = ~(*pCrcVal);
}
#if 0 //this function is caculate the crc of the DSP c6 hex file and add the crc to the end.
unsigned int coffFileGetCrc(unsigned char *pSrc)
{
	unsigned int crcVal = 0;
	unsigned int *pCrcVal = &crcVal;
	unsigned char *pSrcChar = pSrc;
	unsigned char *pSrcCharGetSize = pSrc;

	unsigned int entryAddrBytes = 4;
	unsigned int sectionDestBytes = 4;
	unsigned int sectionSizeBytes = 4;
	unsigned int sectionSize = 0;
	unsigned int endBytes = 4;

	int index = 0;
	crc32_initCrc(pCrcVal);

	sectionSize = entryAddrBytes;
	pSrcCharGetSize += entryAddrBytes; //entry address(4 bytes)

	while (sectionSize != 0)
	{
		for (index = 0; index < sectionSize; index++)
		{

			crc32_updateCrc(pCrcVal, pSrcChar);
			pSrcChar++;
		}
		sectionSize = byteTo32bits(pSrcCharGetSize);
		if (sectionSize != 0)
		{
			//key point.
			sectionSize = ((sectionSize + 3) / 4) * 4;
			sectionSize += sectionSizeBytes;
			sectionSize += sectionDestBytes;
		}
		pSrcCharGetSize += sectionSize;
	}

	// key point.the code data will be end with 0x00000000.
	for (index = 0; index < endBytes; index++)
	{
		crc32_updateCrc(pCrcVal, pSrcChar);
		pSrcChar++;
	}

	crc32_finalCrc(pCrcVal);

	return (*pCrcVal);
}
#endif
int coffFileCheckCrc(unsigned char *pSrc)
{
	int retVal = 0;
	unsigned int crcVal = 0;
	unsigned int *pCrcVal = &crcVal;
	unsigned char *pSrcChar = pSrc;
	unsigned char *pSrcCharGetSize = pSrc;

	unsigned int entryAddrBytes = 4;
	unsigned int sectionDestBytes = 4;
	unsigned int sectionSizeBytes = 4;
	unsigned int sectionSize = 0;
	unsigned int endBytes = 4;
	unsigned int originalCrc = 0;

	int index = 0;
	crc32_initCrc(pCrcVal);

	sectionSize = entryAddrBytes;
	pSrcCharGetSize += entryAddrBytes; //entry address(4 bytes)

	while (sectionSize != 0)
	{
		for (index = 0; index < sectionSize; index++)
		{

			crc32_updateCrc(pCrcVal, pSrcChar);
			pSrcChar++;
		}
		sectionSize = byteTo32bits(pSrcCharGetSize);
		if (sectionSize != 0)
		{
			//key point.
			sectionSize = ((sectionSize + 3) / 4) * 4;
			sectionSize += sectionSizeBytes;
			sectionSize += sectionDestBytes;
		}
		pSrcCharGetSize += sectionSize;
	}

	// key point.the code data will be end with 0x00000000.
	for (index = 0; index < endBytes; index++)
	{
		crc32_updateCrc(pCrcVal, pSrcChar);
		pSrcChar++;
	}

	crc32_finalCrc(pCrcVal);

	//printf("the crc is %x\n", *pCrcVal);
	//get the original crcVal;
	for (index = 0; index < 4; index++)
	{
		originalCrc <<= 8;
		originalCrc |= (*pSrcChar);
		pSrcChar++;
	}
	//printf("the crc is %x\n", originalCrc);
	if (originalCrc == (*pCrcVal))
	{
		retVal = 0;
	}
	else
	{
		retVal = -1;
	}
	return (retVal);
}

#if 0 //this function test the processor is big or little endian
/*
 * if return 0,the processor is big endian model else is little.
 */
int endianTest()
{
	typedef union __tagEndianTest
	{
		unsigned int uint32_data;
		unsigned char uint8_data[4];
	}endianTest_t;
	int retVal=0;
	endianTest_t testVal;
	testVal.uint32_data=0x20304050;
	if(testVal.uint8_data[0]==0x20)
	{
		retVal=1;
		printf("little endian model\n");
	}
	else if(testVal.uint8_data[0]==0x50)
	{
		retVal=0;
		printf("big endian model\n");
	}
	else
	{
		printf("the first data=0x%02x\n",testVal.uint8_data[0]);
	}

	return(retVal);
}
#endif

