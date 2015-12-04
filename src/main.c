#include <stdio.h>
#include <stdlib.h>
#include <platform.h>
#include <string.h>

#define DEF_INIT_CONFIG_UART_BAUDRATE 115200
#define BOOT_MAGIC_ADDR 0x87FFFC	/* boot address in the last word of L2 memory */
#define MAGIC_NUMBER_ADDR 0x81FFE0
#define PLLM_ADDR 0x81FFE4
#if 1
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
#endif
void wait_and_start(void)
{
	void (*entry)();
	*(uint32_t *) (0x1087ffa4) = 0x33aa55aa;
	*((unsigned int *) BOOT_MAGIC_ADDR) = 0;
	while (*((unsigned int *) BOOT_MAGIC_ADDR) == 0)
		;
	entry = (void (*)()) (*((unsigned int *) BOOT_MAGIC_ADDR));
	(*entry)();
}

void main(void)
{
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

	*(uint32_t *) (0x1087fff4) = 0x33aa55aa;
	*(uint32_t *) (0x1087fff0) = 0x77ff88ff;
	*(uint32_t *) (0x1087ffe0) = 0x99ffeeff;

	platform_init(&flags, &config);
	memset(&flags, 0, sizeof(flags));
	flags.ddr = 0x1;
	platform_init(&flags, &config);

	platform_uart_init();
	platform_uart_set_baudrate(DEF_INIT_CONFIG_UART_BAUDRATE);

	*(uint32_t *) (0x1087ffa4) = 0x33aa55aa;
	*(uint32_t *) (0x1087ffa0) = 0x77ff88ff;
	*(uint32_t *) (0x1087ffb0) = 0xbbffeeff;
	/* wait for code download from PCIe driver and jump to entry point */
	write_uart("hello DSPC8681\r\n");
	write_uart("hello DSPC8681\n\r");
	*(uint32_t *) (0x1087ffd4) = 0x33aa55aa;
	*(uint32_t *) (0x1087ffd0) = 0x77ff88ff;
	*(uint32_t *) (0x1087ffc0) = 0xbbffeeff;
	wait_and_start();

}
