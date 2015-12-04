/*
 * crc32.h
 *
 *  Created on: Oct 20, 2015
 *      Author: spark
 */

#ifndef INC_CRC32_H_
#define INC_CRC32_H_

void crc32_getCrc(const unsigned char *pSrc, unsigned int srcLength,
		unsigned int *pCrcVal);

unsigned int coffFileGetCrc(unsigned char *pSrc);

int coffFileCheckCrc(unsigned char *pSrc);

#endif /* INC_CRC32_H_ */
