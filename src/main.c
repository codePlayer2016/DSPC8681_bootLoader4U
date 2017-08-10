#include <stdio.h>
#include <stdlib.h>
#include <platform.h>
#include <string.h>

#include "crc32.h"

#define DEVICE_REG32_W(x,y)   *(volatile uint32_t *)(x)=(y)
#define DEVICE_REG32_R(x)    (*(volatile uint32_t *)(x))

#define MAGIC_ADDR          		(0x0087FFFC)

#define CHIP_LEVEL_REG  0x02620000
#define KICK0           (CHIP_LEVEL_REG + 0x0038)
#define KICK1           (CHIP_LEVEL_REG + 0x003C)
// global Magic address
#define GBOOT_MAGIC_ADDR(coreNum)			((1<<28) + ((coreNum)<<24) + (MAGIC_ADDR))
#define CORE0_MAGIC_ADDR                   (0x1087FFFC)
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

#define DSP_GETCODE_RESET		(0x00000001U)
#define DSP_GETCODE_FINISH 		(0x00000010U)
#define DSP_GETCODE_FAIL		(0x00000000U)

#define DSP_CRCCHECK_SUCCESSFUL (0x00000100U)
#define DSP_CRCCHECK_FAIL		(0x00000000U)

#define DSP_GETENTRY_FINISH 	(0x00001000U)
#define DSP_GETENTRY_FAIL		(0x00000000U)

#define DSP_RUN_READY			(0x00010000U)
#define DSP_RUN_FAIL			(0x00000000U)

//cyx
#define DSP_TEST_SET           (0x00100000U)

// the PC_PushCodeStatus Register value range.
#define PC_PUSHCODE_RESET		(0x00000001U)
#define PC_PUSHCODE_FINISH      (0x00000010U)
#define PC_PUSHCODE_FAIL		(0x00000000U)
// Register Length
#define REG_LEN					(2*1024*4U)

#define BOOT_MAGIC_NUMBER       (0xBABEFACE)

//#define TEST_ADDR (0x0087fff8)
#define GTEST_ADDR(coreNum) (1<<28 + coreNum<<24 + TEST_ADDR)
#if 0
typedef struct _tagRegisterTable
{
	// control registers. (4k)
	uint32_t DPUBootControl;
	uint32_t SetMultiCoreBootControl;
	uint32_t MultiCoreBootControl;
	uint32_t writeControl;
	uint32_t readControl;
	uint32_t DSPgetCodeControl;
	uint32_t reserved0[0x1000 / 4 - 6];

	// status registers. (4k)
	uint32_t DPUBootStatus;
	uint32_t writeStatus;
	uint32_t readStatus;
	uint32_t DSPgetCodeStatus;
	uint32_t reserved1[0x1000 / 4 - 4];
}registerTable;
#endif
typedef struct _tagRegisterTable
{
	// control registers. (4k)
	uint32_t DPUBootControl;
	uint32_t SetMultiCoreBootControl;
	uint32_t MultiCoreBootControl;
	uint32_t writeControl;
	uint32_t readControl;
	uint32_t reserved0[5];
	uint32_t pushCodeControl;
	uint32_t reserved1[0x1000 / 4 - 11];

	// status registers. (4k)
	uint32_t DPUBootStatus;
	uint32_t writeStatus;
	uint32_t readStatus;
	uint32_t reserved2[4];
	uint32_t pushCodeStatus;
	uint32_t reserved3[0x1000 / 4 - 8];
} registerTable;
char printMessage[255];

extern cregister volatile unsigned int DNUM;
extern far uint32_t _c_int00;

#endif

#if 1
static int pollValue(uint32_t *pAddress, uint32_t pollVal, uint32_t maxPollCount);
static int pushSectionData(uint32_t destAddr, uint32_t* pSrcAddr, uint32_t length);
static int putData(uint32_t *DSPCodeAddr, uint32_t *pBootEntryAddr);
uint32_t byteTo32bits(uint8_t *pDspCode);
#endif

#define DEBUG_PROCESS
#ifdef DEBUG_PROCESS
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
#else
void write_uart(char *msg)
{
}
#endif

int main(void)
{

	int retVal = 0;
	void (*core0EntryAddr)();

	if (0 == DNUM)
	{

		uint32_t BootEntryAddr = 0;
		uint32_t *pBootEntryAddr = &BootEntryAddr;
		uint8_t *pCodeStartAddr = NULL;

		int pcPushCodeFlag = 0;
		// 1.init the platform(all the U) in the core0

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

		// 2.DSP platform is init ready.
		registerTable *pRegisterTable = (registerTable *) C6678_PCIEDATA_BASE;

		pRegisterTable->MultiCoreBootControl |= 0x1;
		pRegisterTable->DPUBootControl |= DSP_INIT_READY;

		write_uart("DSPC8681 Platform inition successful\n\r");

		uint32_t Core0L2 = (0x1087ffff - 8 * sizeof(uint32_t));
		memset((uint8_t *) Core0L2, 0, 8 * sizeof(uint32_t));

		int coreIndex = 0;
		int coreMaxNum = 2; // todo: make this var macro.


		// TODO:
		// improve,we can recode the status of every code, and should not return if one core boot failed.
		//pRegisterTable->pushCodeControl = DSP_GETCODE_RESET;
		for (coreIndex = 0; coreIndex < coreMaxNum; coreIndex++)
		{

			//* 1.updateReadBuffer(resetBeforRead).
			pRegisterTable->pushCodeControl = DSP_GETCODE_RESET;
			//* 2.waitPcWrite
			pcPushCodeFlag = pollValue(&(pRegisterTable->pushCodeStatus), PC_PUSHCODE_FINISH, 0x7fffffff);
			if (pcPushCodeFlag == 0)
			{
//				sprintf(printMessage, "coreId=%x,waitPcWriteSuccessful\n\r", coreIndex);
//				write_uart(printMessage);
				//pRegisterTable->DPUBootControl |= DSP_GETCODE_FINISH;
			}
			else
			{
				//pRegisterTable->DPUBootControl |= DSP_GETCODE_FAIL;
				retVal = -1;
				sprintf(printMessage, "coreId=%x,waitPcWriteFailed\n\r", coreIndex);
				write_uart(printMessage);
				return (retVal);
			}
			//* 3. readInBuffer()
			// TODO:
			// add crc check.
			///load the code to the u0-u7 ddr and write the start addr to the magic of the core1-core7 and triggle it.
			// LOOKOUT:
			// Cache operator.
#if 1
			{
				*pBootEntryAddr = 0;
				pCodeStartAddr = (uint8_t *) (REG_LEN + C6678_PCIEDATA_BASE);
				putData((uint32_t *) pCodeStartAddr, pBootEntryAddr);

				if (0 == coreIndex)
				{
					if (*pBootEntryAddr != 0)
					{
						core0EntryAddr = (void (*)()) (*(unsigned *) pBootEntryAddr);
					}
					else
					{
						retVal = -2;
//						sprintf(printMessage, "coreId=%x,*pBootEntryAddr = %x,u0 load code failed\n\r", coreIndex, *pBootEntryAddr);
//						write_uart(printMessage);
						return (retVal);
					}
				}
				else
				{

					if (*pBootEntryAddr != 0)
					{
						DEVICE_REG32_W(GBOOT_MAGIC_ADDR(coreIndex), *pBootEntryAddr);
						platform_delay(1000);
						DEVICE_REG32_W(IPC_INT_ADDR(coreIndex), 1);
						//platform_delay(1000000);
					}
					else
					{
						retVal = -2;

						sprintf(printMessage, "coreId=%x,*pBootEntryAddr = %x,u0 load code failed\n\r", coreIndex, *pBootEntryAddr);
						write_uart(printMessage);
						return (retVal);
					}
				}
//				sprintf(printMessage, "coreId=%x,*pBootEntryAddr = %x,u0 load code successful\n\r", coreIndex, *pBootEntryAddr);
//				write_uart(printMessage);
			}
#endif
			//* 4.updateReadBuffer(finished);
			pRegisterTable->pushCodeControl = DSP_GETCODE_FINISH;
//			write_uart("updateReadBuffer_finished\n\r");

			//* 5. waitWriteBufferReset();
			pcPushCodeFlag = pollValue(&(pRegisterTable->pushCodeStatus), PC_PUSHCODE_RESET, 0x7fffffff);
			if (pcPushCodeFlag == 0)
			{
//				sprintf(printMessage, "coreId=%x,waitPcResetSuccessful\n\r", coreIndex);
//				write_uart(printMessage);
			}
			else
			{
				retVal = -1;
				//sprintf(printMessage, "coreId=%x,waitPcResetFailed\n\r", coreIndex);
				//write_uart(printMessage);
				return (retVal);
			}

		}
		pRegisterTable->pushCodeControl = DSP_GETCODE_RESET;

		pRegisterTable->SetMultiCoreBootControl = 0xff;
		platform_delay(10000000);

		// wait the subCore to start.
		// TODO: set the *((uint32_t*) (Core0L2 + coreIndex * 4)) to 0 before polling that address.
		// core1-core7 will write 1 to the Core0L2 + coreIndex * 4 after it boot.
		uint32_t temp;
		Core0L2 = (0x1087ffff - 8 * 4);
		pRegisterTable->MultiCoreBootControl = 0x00000001;		// core 0 have boot.
		for (coreIndex = 1; coreIndex < coreMaxNum; coreIndex++)
		{
			*pBootEntryAddr = DEVICE_REG32_R(GBOOT_MAGIC_ADDR(coreIndex));
			if (*pBootEntryAddr != 0)
			{
				temp = *((uint32_t*) (Core0L2 + coreIndex * 4));
				pRegisterTable->MultiCoreBootControl |= (temp << coreIndex);
			}
			else
			{
				retVal = -3;
				sprintf(printMessage, "coreIndex=%x,MagicAddr=%x,boot Failed\n\r", coreIndex, *pBootEntryAddr);
				write_uart(printMessage);
				return (retVal);
			}
			sprintf(printMessage, "coreIndex=%x,MagicAddr=%x,boot successful\n\r", coreIndex, *pBootEntryAddr);
			write_uart(printMessage);
		}
		// TODO: pRegisterTable->DPUBootControl,should express every core status.
		//pRegisterTable->DPUBootControl |= DSP_GETENTRY_FINISH;
		//pRegisterTable->DPUBootControl |= DSP_RUN_READY;
		(*core0EntryAddr)();

#if 0
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
#endif

	}
}

#if 1

int pollValue(uint32_t *pAddress, uint32_t pollVal, uint32_t maxPollCount)
{
	int retVal = 0;
	uint32_t loopCount = 0;
	uint32_t stopPoll = 0;
	uint32_t realTimeVal = 0;

	for (loopCount = 0; (loopCount < maxPollCount) && (stopPoll == 0); loopCount++)
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

	//sprintf(printMessage, "the pBootEntryAddr value is %x\n\r", *pBootEntryAddr);
	//write_uart(printMessage);

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

int writeTOdsp()
{
	//updateWriteBuffer(reset);
	//waitDSPreadBufferReset();
	//writeOutBuffer();
	//updateWriteBuffer(writeFinished);
	//waitDSPreadFinished();
	return 0;
}
int readFromPC()
{
	//updateReadBuffer(reset);
	//waitPCwriteBuffer();
	//readInBuffer();
	//updateReadBuffer(readFinished);
	//waitWriteBufferReset();
	return 0;
}

#endif
