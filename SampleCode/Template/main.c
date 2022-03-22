/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "project_config.h"

#include "SPI_Flash.h"
#include "diskio.h"
#include "ff.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/


/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

FATFS g_sFatFs;
FATFS *fs;	
FIL f1;
DWORD fre_clust;
DWORD acc_size;                         /* Work register for fs command */
WORD acc_files, acc_dirs;
FILINFO Finfo;
TCHAR _Path[3] = { '0', ':', 0 };

// char Line[256];                         /* Console input buffer */
#if FF_USE_LFN >= 1
char Lfname[512];
#endif

// uint16_t counter = 0;
UINT uiWriteLen;
UINT uiReadLen;

uint8_t Buffer_Block_Rx[512] = {0};
const uint8_t Power_Buffer[] = "Added selection of character encoding on the file. (_STRF_ENCODE)Added f_closedir().\
Added forced full FAT scan for f_getfree(). (_FS_NOFSINFO)Added forced mount feature with changes of f_mount().\
Improved behavior of volume auto detection.Improved write throughput of f_puts() and f_printf().\
Changed argument of f_chdrive(), f_mkfs(), disk_read() and disk_write().Fixed f_write() can be truncated when the file size is close to 4GB.\
Fixed f_open, f_mkdir and f_setlabel can return incorrect error code.The STM32F427xx and STM32F429xx devices are based on the \
high-performance ARMRCortexR-M4 32-bit RISC core operating at a frequency of up to 180 MHz. The Cortex-M4 core features a Floating \
point unit (FPU) single precision which supports all ARMR single-precision data-processing instructions and data types. It also implements \
a full set of DSP instructions and a memory protection unit (MPU) which enhances application security.\
The STM32F427xx and STM32F429xx devices incorporate high-speed embedded memories (Flash memory up to 2 Mbyte, up to 256 kbytes of \
SRAM), up to 4 Kbytes of backup SRAM, and an extensive range of enhanced I/Os and peripherals connected to two APB buses, two AHB buses \
and a 32-bit multi-AHB bus matrix.All devices offer three 12-bit ADCs, two DACs, a low-power RTC, twelve general-purpose 16-bit timers \
including two PWM timers for motor control, two general-purpose 32-bit timers.";

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(uint8_t *pucBuff, int nBytes)
{
	#if 1
    uint16_t i = 0;	
    for ( i = 0; i < nBytes; i++)
    {
        pucBuff[i] = 0x00;
    }	
	#else	//extra 20 bytes , with <string.h>
	memset(pucBuff, 0, nBytes * (sizeof(pucBuff[0]) ));
	#endif
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

void delay(uint16_t dly)
{
/*
	delay(100) : 14.84 us
	delay(200) : 29.37 us
	delay(300) : 43.97 us
	delay(400) : 58.5 us	
	delay(500) : 73.13 us	
	
	delay(1500) : 0.218 ms (218 us)
	delay(2000) : 0.291 ms (291 us)	
*/

	while( dly--);
}


void delay_ms(uint16_t ms)
{
	TIMER_Delay(TIMER0, 1000*ms);
}


/*
	WP : PA4 (D9)
	HOLD : PA5 (D8)
*/
void SPIFlash_WP_HOLD_Init (void)
{
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA4MFP_Msk)) | (SYS_GPA_MFPL_PA4MFP_GPIO);
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA5MFP_Msk)) | (SYS_GPA_MFPL_PA5MFP_GPIO);
	
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT5, GPIO_MODE_OUTPUT);	

	PA4 = 1;
	PA5 = 1;		
}

void GPIO_Init (void)
{
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB14MFP_Msk)) | (SYS_GPB_MFPH_PB14MFP_GPIO);
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB15MFP_Msk)) | (SYS_GPB_MFPH_PB15MFP_GPIO);
	
    GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);	
}


void put_rc(FRESULT rc)
{
    uint32_t i;
    const TCHAR *p =
        _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")
        _T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
        _T("NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
        _T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0");

    for (i = 0; (i != (UINT)rc) && *p; i++)
    {
        while (*p++) ;
    }

    printf(_T("rc=%u FR_%s\n"), (UINT)rc, p);
}

FRESULT get_free (char* path)	//Show logical drive status
{
	// DWORD fre_sect, tot_sect;
	const BYTE ft[] = {0, 12, 16, 32};

	FRESULT res = FR_INVALID_PARAMETER;	
	res = f_getfree(path, (DWORD*)&fre_clust, &fs);
	put_rc(res);

    // tot_sect = (fs->n_fatent - 2) * fs->csize;
    // fre_sect = fre_clust * fs->csize;
	// printf("%10lu KiB total drive space.\r\n%10lu KiB available.\r\n\r\n", tot_sect / 2, fre_sect / 2);

	printf("FAT type = FAT%d\nBytes/Cluster = %d\nNumber of FATs = %d\n"
			"Root DIR entries = %d\nSectors/FAT = %d\nNumber of clusters = %d\n"
			"FAT start (lba) = %d\nDIR start (lba,clustor) = %d\nData start (lba) = %d\n\n...",
			ft[fs->fs_type & 3], fs->csize * 512UL, fs->n_fats,
			fs->n_rootdir, fs->fsize, fs->n_fatent - 2,
			fs->fatbase, fs->dirbase, fs->database
			);
	acc_size = acc_files = acc_dirs = 0;
// #if FF_USE_LFN >= 1
// 	Finfo.lfname = Lfname;
// 	Finfo.lfsize = sizeof(Lfname);
// #endif

    return res;
}

FRESULT scan_files (char* path)
{
    DIR dirs;
    FRESULT res;
    BYTE i;
    char *fn;

    if ((res = f_opendir(&dirs, path)) == FR_OK)
    {
        i = strlen(path);
        while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0])
        {
            if (FF_FS_RPATH && Finfo.fname[0] == '.') continue;
// #if FF_USE_LFN >= 1
            // fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
// #else
            fn = Finfo.fname;
// #endif
            if (Finfo.fattrib & AM_DIR)
            {
                acc_dirs++;
                *(path+i) = '/';
                strcpy(path+i+1, fn);
                res = scan_files(path);
                *(path+i) = '\0';
                if (res != FR_OK) break;
            }
            else
            {
                printf("%s/%s\r\n", path, fn);
                acc_files++;
                acc_size += Finfo.fsize;
            }
        }
    }

    return res;
}


void Fatfs_scan_files(void)
{

	FRESULT res = FR_INVALID_PARAMETER;

	get_free(_Path);
	put_rc(res);

	res = scan_files(_Path);
	put_rc(res);

	printf("\r%d files, %d bytes.\n%d folders.\n"
			"%d KB total disk space.\n%d KB available.\n",
			acc_files, acc_size, acc_dirs,
			(fs->n_fatent - 2) * (fs->csize / 2), fre_clust * (fs->csize / 2)
			);
}

void Fatfs_ReadWriteTest(char* File , uint8_t* Buffin , uint8_t* Buffout , uint32_t Len)
{
	uint32_t i = 0 ;
	FRESULT res = FR_INVALID_PARAMETER;

	printf(">>>>>Fatfs_ReadWriteTest  START\r\n\r\n");
	res = f_open(&f1, File, FA_CREATE_ALWAYS | FA_WRITE  | FA_READ);
	put_rc(res);
	// printf("<Fatfs_Write>	f_open res : %d \r\n" , res);
	
	if(res !=FR_OK)
	{
		printf("<Fatfs_Write>	f_open NG  \r\n");
	}	
	else
	{
		printf("<Fatfs_Write>	f_open OK  \r\n");
		printf("<Fatfs_Write>	Write Data to File!!!\r\n");
		res = f_write(&f1, Buffin, Len, &uiWriteLen);
		put_rc(res);
		if(res !=FR_OK)
		{
			printf("<Fatfs_Write>	Write [%s] NG\r\n" , File);
		}		
		else
		{
			printf("<Fatfs_Write>	Write [%s] OK\r\n" , File);
		}

		res = f_close(&f1);
		put_rc(res);
		if(res != FR_OK)
		{
			printf("<Fatfs_Write>	f_close NG\r\n\r\n");
		}
		else
		{
			printf("<Fatfs_Write>	f_close OK\r\n\r\n");
		}
	}	

	res = f_open(&f1, File, FA_READ);
	put_rc(res);
	if(res != FR_OK)
	{
		printf("<Fatfs_Read>	f_open NG  \r\n");
	}	
	else
	{
		printf("\r\n<Fatfs_Read>	f_open OK  \r\n");
		printf("<Fatfs_Read>	Read Data to Buffout!!!\r\n");

		res = f_read(&f1, Buffout, Len, &uiReadLen);
		put_rc(res);
		if(res != FR_OK)
		{
			printf("<Fatfs_Read>	Read [%s] NG\r\n",File);
		}		
		else
		{
			printf("<Fatfs_Read>	Read [%s] OK\r\n",File);
		}

		res = f_close(&f1);
		put_rc(res);
		if(res != FR_OK)
		{
			printf("<Fatfs_Read>	f_close NG\r\n\r\n");
		}
		else
		{
			printf("<Fatfs_Read>	f_close OK\r\n\r\n");
		}
	}	

	printf("uiReadLen = %d \r\n",Len);
	printf("Data = ");
	printf("\r\n");
	for(i=0;i<Len;i++)
	{
		printf("%c",Buffout[i]);
	}
	printf("\r\n\r\n");

	printf("<<<<<Fatfs_ReadWriteTest  FINISH\r\n");	
}

void Fatfs_Test(void)
{
	reset_buffer(Buffer_Block_Rx , 512);
	Fatfs_ReadWriteTest("1218DATA.TXT", (uint8_t*) Power_Buffer,Buffer_Block_Rx,512);

	Fatfs_ReadWriteTest("0311DATA.TXT", (uint8_t*) Power_Buffer,Buffer_Block_Rx,512);

}

void Fatfs_Init(void)
{
	unsigned int MidDid;	
	BYTE work[FF_MAX_SS];
	FRESULT res = FR_INVALID_PARAMETER;	

    SpiInit();

    /* Read MID & DID */
    MidDid = SpiReadMidDid();
    printf("\r\nMID and DID = 0x%4X\r\n", MidDid);

	#if 0
	printf("erase flash\r\n");
	SpiChipErase();	
	#endif

	res = f_mount(&g_sFatFs, _Path ,1 );
	put_rc(res);
	if(res != FR_OK)	//Mount Logical Drive 
	{
		printf("f_mount NG , start erase flash\r\n");

		//if mount , erase flash and reset fatfs
		SpiChipErase();	

		res = f_mkfs(_Path , 0 , work , sizeof(work));
		printf("f_mkfs process\r\n\r\n");

	}
	else
	{
		printf("f_mount OK\r\n\r\n");
	}
}

void process(void)
{
	if (is_flag_set(flag_process_1))
	{
		set_flag(flag_process_1 , DISABLE);
		reset_buffer(Buffer_Block_Rx , 512);
		Fatfs_ReadWriteTest("20220311.TXT", (uint8_t*) Power_Buffer,Buffer_Block_Rx,512);			
	}
	if (is_flag_set(flag_process_2))
	{
		set_flag(flag_process_2 , DISABLE);
		reset_buffer(Buffer_Block_Rx , 512);
		Fatfs_ReadWriteTest("customfile_aaa.TXT", (uint8_t*) Power_Buffer,Buffer_Block_Rx,512);
	}
	if (is_flag_set(flag_process_3))
	{
		set_flag(flag_process_3 , DISABLE);
		reset_buffer(Buffer_Block_Rx , 512);
		Fatfs_ReadWriteTest("test_longlonglonglong_files_name_need_to_enable_FF_USE_LFN_code_size_will_increase.TXT", (uint8_t*) Power_Buffer,Buffer_Block_Rx,512);
	}
	if (is_flag_set(flag_process_4))
	{
		set_flag(flag_process_4 , DISABLE);
		Fatfs_scan_files();
	}
}

void TMR1_IRQHandler(void)
{
//	static uint32_t LOG = 0;

	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
//        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PB14 ^= 1;
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1' :
				printf("set flag_process_1\r\n");		
				set_flag(flag_process_1 ,ENABLE);
				break;	
			case '2' :	
				printf("set flag_process_2\r\n");		
				set_flag(flag_process_2 ,ENABLE);
				break;	
			case '3' :	
				printf("set flag_process_3\r\n");		
				set_flag(flag_process_3 ,ENABLE);
				break;	
			case '4' :	
				printf("set flag_process_4\r\n");		
				set_flag(flag_process_4 ,ENABLE);
				break;	

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();
			
				break;		
			
		}
	}
}

void UART02_IRQHandler(void)
{

    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
            UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    NVIC_EnableIRQ(UART02_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif	

}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);	

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);	

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

//    CLK_EnableModuleClock(PDMA_MODULE);

	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);

    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);

//    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);
//    CLK_EnableModuleClock(SPI0_MODULE);

    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk)) |
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

	/* 
		SPI0 flash , 
		SPI0_FLASH_CLK : PA.2
		SPI0_FLASH_MISO : PA.1
		SPI0_FLASH_MOSI : PA.0
		SPI0_FLAH_NSS0 : PA.3
	*/
//    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk| SYS_GPA_MFPL_PA1MFP_Msk| SYS_GPA_MFPL_PA0MFP_Msk);
//    SYS->GPA_MFPL |= SYS_GPA_MFPL_PA3MFP_SPI0_SS| SYS_GPA_MFPL_PA2MFP_SPI0_CLK| SYS_GPA_MFPL_PA1MFP_SPI0_MISO| SYS_GPA_MFPL_PA0MFP_SPI0_MOSI ;


   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M031 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	UART0_Init();
	GPIO_Init();
	TIMER1_Init();

	SPIFlash_WP_HOLD_Init();

	Fatfs_Init();
	Fatfs_Test();
	
    /* Got no where to go, just loop forever */
    while(1)
    {
		process();

    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
