#include <stdio.h>
#include <stdlib.h>
#include <platform.h>
#include <string.h>

#include "crc32.h"

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))

#define MAGIC_ADDR          		(0x0087FFFC)

// global Magic address
#define GBOOT_MAGIC_ADDR(coreNum)			((1<<28) + ((coreNum)<<24) + (MAGIC_ADDR))
// global IPC interrupt address
#define IPC_INT_ADDR(coreNum)				(0x02620240 + ((coreNum)*4))

#define DEF_INIT_CONFIG_UART_BAUDRATE 115200
#define BOOT_MAGIC_ADDR 0x87FFFC	/* boot address in the last word of L2 memory */
#define MAGIC_NUMBER_ADDR 0x81FFE0
#define PLLM_ADDR 0x81FFE4

#if 1
#define BLOCK_TRANSFER_SIZE         (0x100)

#define C6678_DDR3_BASE     (0x80000000U)
#define C6678_PCIEDATA_BASE (0x60000000U)

// the DSPBootStaus Register value range.
#define DSP_INIT_READY 			(0x00000001U)
#define DSP_INIT_MASK 			(0x00000001U)

#define DSP_GETCODE_FINISH 		(0x00000010U)
#define DSP_GETCODE_FAIL		(0x00000000U)

#define DSP_CRCCHECK_SUCCESSFUL (0x00000100U)
#define DSP_CRCCHECK_FAIL		(0x00000000U)

#define DSP_GETENTRY_FINISH 	(0x00001000U)
#define DSP_GETENTRY_FAIL		(0x00000000U)

#define DSP_RUN_READY			(0x00010000U)
#define DSP_RUN_FAIL			(0x00000000U)

// the PC_PushCodeStatus Register value range.
#define PC_PUSHCODE_FINISH      (0x00000011U)
#define PC_PUSHCODE_FAIL		(0x00000000U)
// Register Length
#define REG_LEN					(2*1024*4U)

typedef struct _tagRegisterTable
{
	// control registers. (4k)
	uint32_t DPUBootControl;
	uint32_t writeControl;
	uint32_t readControl;
	uint32_t reserved0[0x1000 / 4 - 3];

	// status registers. (4k)
	uint32_t DPUBootStatus;
	uint32_t writeStatus;
	uint32_t readStatus;
	uint32_t reserved1[0x1000 / 4 - 3];
} registerTable;

char printMessage[255];

extern cregister volatile unsigned int DNUM;

#endif

#if 1
static int pollValue(uint32_t *pAddress, uint32_t pollVal,
		uint32_t maxPollCount);
static int pushSectionData(uint32_t destAddr, uint32_t* pSrcAddr,
		uint32_t length);
static int putData(uint32_t *DSPCodeAddr, uint32_t *pBootEntryAddr);
uint32_t byteTo32bits(uint8_t *pDspCode);
#endif

void write_uart(char* msg)
{
	uint32_t i;
	uint32_t msg_len = strlen(msg);

	/* Write the message to the UART */
	for (i = 0; i < msg_len; i++)
	{
		platform_uart_write(msg[i]);
	}
}

void wait_and_start(registerTable *pRegisterTable)
{
	void (*entry)();
	while (*((unsigned int *) BOOT_MAGIC_ADDR) == 0)
		;
	entry = (void (*)()) (*((unsigned int *) BOOT_MAGIC_ADDR));
	pRegisterTable->DPUBootControl |= DSP_GETENTRY_FINISH;
	(*entry)();
}

void subCoreBootStart()
{
	// display the coreN boot successufl.
	// check the magic address in the circle.
	// if magic address is not zero,jump to it.
	void (*entry)();

	sprintf(printMessage, "core%d boot successful\n\r", DNUM);
	write_uart(printMessage);

	while (0 == *(int *) BOOT_MAGIC_ADDR)
	{
		;
	}

	sprintf(printMessage, "core%d 's boot Address=%x\n\r", BOOT_MAGIC_ADDR);
	//entry = (void (*)()) (*(unsigned *) BOOT_MAGIC_ADDR);
	//(*entry)();
	while (1)
	{
		;
	}

}

void main(void)
{
	if (0 == DNUM)
	{
		uint32_t BootEntryAddr = 0;
		uint32_t *pBootEntryAddr = &BootEntryAddr;

		int pcPushCodeFlag = 0;
#if 1
		platform_init_flags flags;
		platform_init_config config;

		/* Platform initialization */
		flags.pll = 0x1;
		flags.ddr = 0x0;
		flags.tcsl = 0x1;
		flags.phy = 0x1;
		flags.ecc = 0x1;

		/* Original pllm configuraion : default 0 -> 1 GHz */
		config.pllm = 0;

		/* Check if external pllm is set*/
		if (*((unsigned int *) MAGIC_NUMBER_ADDR) == 0xFACE13FE)
		{
			config.pllm = *(unsigned int *) PLLM_ADDR;
		}

		platform_init(&flags, &config);
		memset(&flags, 0, sizeof(flags));
		flags.ddr = 0x1;
		platform_init(&flags, &config);

		platform_uart_init();
		platform_uart_set_baudrate(DEF_INIT_CONFIG_UART_BAUDRATE);
#endif
//	wait_and_start();

		registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;
		pRegisterTable->DPUBootControl = 0x00000000;
		write_uart("Init the DSP finished\n\r");

		// 2. write the DSPInitReadyFlag
		pRegisterTable->DPUBootControl |= DSP_INIT_READY;

		// 3. wait for the PC put code to DDR3. and return the status.
		pcPushCodeFlag = pollValue(&(pRegisterTable->DPUBootStatus),
				PC_PUSHCODE_FINISH, 0x7fffffff);
		if (pcPushCodeFlag == 0)
		{
			write_uart("Get the code successful\n\r");

			sprintf(printMessage, "DPUBootStatus=%x\n\r",
					pRegisterTable->DPUBootStatus);
			write_uart(printMessage);
			pRegisterTable->DPUBootControl |= DSP_GETCODE_FINISH;
		}
		else
		{
			write_uart("Get the code error:time over\n\r");
			pRegisterTable->DPUBootControl |= DSP_GETCODE_FAIL;
		}

		// 4. check the code and put the section to the proper address.
		if (pcPushCodeFlag == 0)
		{
			// check the code.
			uint32_t crcResult = 0;
			uint8_t *pCodeAddr = (uint8_t *) (REG_LEN + C6678_PCIEDATA_BASE);

			write_uart("begin to calculate the crc\n\r");

			crcResult = coffFileCheckCrc(pCodeAddr);
			if (crcResult == 0)
			{
				// crc check successful.
				pRegisterTable->DPUBootControl |= DSP_CRCCHECK_SUCCESSFUL;
			}
			else
			{
				// crc check failed.
				pRegisterTable->DPUBootControl |= DSP_CRCCHECK_FAIL;
			}
			sprintf(printMessage, "the crc value is %x\n\r", crcResult);
			write_uart(printMessage);

			if (crcResult == 0)
			{
				putData((uint32_t *) pCodeAddr, pBootEntryAddr);
#if 0
				sprintf(printMessage, "pBootEntryAddr = %x\n\r", pBootEntryAddr);
				write_uart(printMessage);
#endif
			}
			else
			{
			}

		}
		else
		{
		}
		// 5. writes the boot entry address to the subcores MAGICADDR
		if (*pBootEntryAddr != 0)
		{
			sprintf(printMessage, "write %x to sub cores' MAGIC_ADDR\n\r",
					*pBootEntryAddr);
			write_uart(printMessage);

			//DEVICE_REG32_W(MAGIC_ADDR, *pBootEntryAddr);
			unsigned int coreIndex = 0;
			for (coreIndex = 1; coreIndex < 8; coreIndex++)
			{
				DEVICE_REG32_W(GBOOT_MAGIC_ADDR(coreIndex), *pBootEntryAddr);
				platform_delay(100);
				DEVICE_REG32_W(IPC_INT_ADDR(coreIndex), 1);
				platform_delay(500);
			}
#if 0
			sprintf(printMessage, "MAGIC_ADDR = %x\n\r",
					DEVICE_REG32_R(MAGIC_ADDR));
			write_uart(printMessage);
#endif
			//wait_and_start(pRegisterTable);
			wait_and_start(pRegisterTable);
		}
	}
	else
	{
		subCoreBootStart();
	}

	// for no

}
#if 1

int pollValue(uint32_t *pAddress, uint32_t pollVal, uint32_t maxPollCount)
{
	int retVal = 0;
	uint32_t loopCount = 0;
	uint32_t stopPoll = 0;
	uint32_t realTimeVal = 0;

	for (loopCount = 0; (loopCount < maxPollCount) && (stopPoll == 0);
			loopCount++)
	{
		realTimeVal = (*pAddress);
		if (realTimeVal & pollVal)
		//if (realTimeVal == pollVal)
		{
			stopPoll = 1;
		}
		else
		{
		}
	}
	if (loopCount < maxPollCount)
	{
		retVal = 0;
	}
	else
	{
		retVal = -1;
	}

	return (retVal);
}

int pushSectionData(uint32_t destAddr, uint32_t* pSrcAddr, uint32_t length)
{
	int retVal = 0;
	uint32_t *pSrc = pSrcAddr;
	uint32_t *pDest = (uint32_t *) destAddr;

	if (length > 0)
	{
		memcpy(pDest, pSrc, length);
	}
	else
	{
		retVal = -1;
	}

	return (retVal);
}

int putData(uint32_t *DSPCodeAddr, uint32_t *pBootEntryAddr)
{
	int retValue = 0;
	char printMessage[255];
	uint8_t *pDspImg = (uint8_t *) DSPCodeAddr;
	//uint32_t bootEntryAddr = 0;
	uint32_t secStartAddr = 0;
	uint32_t secSize = 0;
	uint32_t tempArray[BLOCK_TRANSFER_SIZE / 4];
	uint32_t secNum = 0;
	uint32_t totalSize = 0;
	uint32_t i = 0, j = 0;
	uint32_t count = 0;
	uint32_t remainder = 0;

	write_uart("putData begin\n\r");
// Get the boot entry address
	*pBootEntryAddr = byteTo32bits(pDspImg);
	pDspImg += 4;

	sprintf(printMessage, "the pBootEntryAddr value is %x\n\r",
			*pBootEntryAddr);
	write_uart(printMessage);

// Get the 1st sect secSize
	secSize = byteTo32bits(pDspImg);
	pDspImg += 4;
	for (secNum = 0; secSize != 0; secNum++)
	{
// adjusts the secSize.
//		if ((secSize / 4) * 4 != secSize)
//		{
//			secSize = ((secSize / 4) + 1) * 4;
//		}
		secSize = ((secSize + 3) / 4) * 4;
		totalSize += secSize;

// Gets the section start address.
		secStartAddr = byteTo32bits(pDspImg);
		pDspImg += 4;

//transfer the section code to DSP.
		count = secSize / BLOCK_TRANSFER_SIZE;
		remainder = secSize - count * BLOCK_TRANSFER_SIZE;
#if 0
		sprintf(printMessage, "secStartAddr=%x,secSize=%x,\n\r", secStartAddr,
				secSize);
		write_uart(printMessage);
#endif
		for (i = 0; i < count; i++)
		{
			for (j = 0; j < BLOCK_TRANSFER_SIZE / 4; j++)
			{
				tempArray[j] = byteTo32bits(pDspImg);
				pDspImg += 4;
			}
			/* Transfer boot tables to DSP */
			pushSectionData(secStartAddr, tempArray, BLOCK_TRANSFER_SIZE);
			secStartAddr += BLOCK_TRANSFER_SIZE;
		}

		for (j = 0; j < remainder / 4; j++)
		{
			tempArray[j] = byteTo32bits(pDspImg);
			pDspImg += 4;
		}
		pushSectionData(secStartAddr, tempArray, remainder);

// Get the next sect secSize
		secSize = byteTo32bits(pDspImg);
		pDspImg += 4;
	}
	return (retValue);
}

uint32_t byteTo32bits(uint8_t *pDspCode)
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
#endif
