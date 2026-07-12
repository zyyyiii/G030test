#ifndef __LCD_H
#define __LCD_H
#include "stdint.h"

#include "main.h"



#define RED  	0xf800
#define GREEN	0x07e0
#define BLUE 	0x001f
#define WHITE	0xffff
#define BLACK	0x0000
#define YELLOW  0xFFE0

#define CYAN    0x07ff
#define BRIGHT_RED 0xf810   

#define GRAY0   0xEF7D   	//灰色0 3165 00110 001011 00101
#define GRAY1   0x8410      	//灰色1      00000 000000 00000
#define GRAY2   0x4208      	//灰色2  1111111111011111

#define X_MAX_PIXEL	        128
#define Y_MAX_PIXEL	        128

//LCD的SPI引脚的定义
#define LCD_CTRL_PORT           GPIOB		//定义TFT数据端口
#define LCD_LED        	      LCD_Pin  //LCD背光--->>TFT --BL
#define LCD_RS         	      MISO_Pin	//MISO--->>TFT --RS/DC
#define LCD_SDA        	      MOSI_Pin	//MOSI--->>TFT --SDA/DIN
#define LCD_SCL        	      SCLK_Pin	//SCK--->>TFT --SCL/SCK

#define LCD_CS_PORT     CSS_GPIO_Port
#define LCD_CS        	CSS_Pin  //MCU_PB11--->>TFT --CS/CE

//液晶控制口置1操作语句宏定义
#define	LCD_CS_SET  	LCD_CS_PORT->BSRR=LCD_CS 
   
#define	LCD_RS_SET  	LCD_CTRL_PORT->BSRR=LCD_RS    
#define	LCD_SDA_SET  	LCD_CTRL_PORT->BSRR=LCD_SDA    
#define	LCD_SCL_SET  	LCD_CTRL_PORT->BSRR=LCD_SCL     
#define	LCD_LED_SET  	LCD_CTRL_PORT->BSRR=LCD_LED   


//液晶控制口置0操作语句宏定义
#define	LCD_CS_CLR  	LCD_CS_PORT->BRR=LCD_CS 
   
#define	LCD_RS_CLR  	LCD_CTRL_PORT->BRR=LCD_RS    
#define	LCD_SDA_CLR  	LCD_CTRL_PORT->BRR=LCD_SDA    
#define	LCD_SCL_CLR  	LCD_CTRL_PORT->BRR=LCD_SCL       
#define	LCD_LED_CLR  	LCD_CTRL_PORT->BRR=LCD_LED 



void LCD_GPIO_Init(void);
void Lcd_WriteIndex(uint8_t Index);
void Lcd_WriteData(uint8_t Data);
void Lcd_WriteReg(uint8_t Index,uint8_t Data);
uint16_t Lcd_ReadReg(uint8_t LCD_Reg);
void Lcd_Reset(void);
void Lcd_Init(void);
void Lcd_Clear(uint16_t Color);
void Lcd_SetXY(uint16_t x,uint16_t y);
void Gui_DrawPoint(uint16_t x,uint16_t y,uint16_t Data);
unsigned int Lcd_ReadPoint(uint16_t x,uint16_t y);
void Lcd_SetRegion(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end);
void LCD_WriteData_16Bit(uint16_t Data);
void showimage(const unsigned char *p);
void Lcd_ReadID(void);
void showimage_farsight(const unsigned char *p);
void Gui_DrawFont_GBK16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s);
void Gui_DrawFont_1616(uint16_t x0, uint16_t y0, uint16_t fc, uint16_t bc,const char tfont16[][32],int num);
#endif

