
#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

/*_____ I N C L U D E S ____________________________________________________*/
#include "NuMicro.h"

/*_____ D E F I N I T I O N S ______________________________________________*/

/*_____ M A C R O S ________________________________________________________*/

/*_____ D E C L A R A T I O N S ____________________________________________*/
	
/*_____ F U N C T I O N S ______------______________________________________*/

void SpiFlash_SendCommandData( uint32_t u32Cmd, uint32_t u32CmdSize);
void SpiFlash_ReadDataByByte(uint8_t * pu8Data);
void SpiFlash_ReadDataByWord(uint8_t * pu8Data);
void SpiFlash_GetStatus(uint8_t * pu8Status);	
uint8_t SpiFlash_WaitIdle(void);	
void SpiFlash_EnableWrite(uint8_t bEnable);	
void SpiFlash_EraseSector(uint32_t u32StartSector);
int32_t SpiFlash_ProgramPage(uint32_t u32StartPage, uint8_t * pu8Data);
int32_t SpiFlash_ReadPage(uint8_t u8ReadMode,uint32_t u32StartPage, uint8_t * pu8Data);
void SpiInit(void);	
void SpiRead(uint32_t addr, uint32_t size, uint32_t buffer);	
void SpiWrite(uint32_t addr, uint32_t size, uint32_t buffer);
void SpiChipErase(void);
uint32_t SpiReadMidDid(void);


#endif	/*__SPI_FLASH_H*/
