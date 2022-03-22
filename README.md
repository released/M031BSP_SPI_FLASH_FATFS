# M031BSP_SPI_FLASH_FATFS
 M031BSP_SPI_FLASH_FATFS

update @ 2022/03/22

1. below is SPI flash pin define ,

	SPI0_FLASH_CLK : PA.2
	
	SPI0_FLASH_MISO : PA.1
	
	SPI0_FLASH_MOSI : PA.0
	
	SPI0_FLAH_NSS0 : PA.3

	WP : PA4 (D9)
	
	HOLD : PA5 (D8)	

2. Initial SPI flash with FATFS v.014b 

	enable FF_USE_LFN to support long file name , side effect : will have extra code size
	
3. below is log message capture

FATFS initial , and create / write / read 2 files 

![image](https://github.com/released/M031BSP_SPI_FLASH_FATFS/blob/main/initial.jpg)

press digit 1 , will  create / write / read one new files

![image](https://github.com/released/M031BSP_SPI_FLASH_FATFS/blob/main/digital_1.jpg)

press digit 2 , will  create / write / read one new files , with long file name

![image](https://github.com/released/M031BSP_SPI_FLASH_FATFS/blob/main/digital_2.jpg)

press digit 3 , will  create / write / read one new files , with long long long file name

![image](https://github.com/released/M031BSP_SPI_FLASH_FATFS/blob/main/digital_3.jpg)

press digit 4 , will  scan current FATFS drive files

![image](https://github.com/released/M031BSP_SPI_FLASH_FATFS/blob/main/digital_4.jpg)


