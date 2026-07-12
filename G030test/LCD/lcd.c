#include "gpio.h"
#include "stdint.h"
#include "lcd.h"
#include "lcd_font.h"
#include <string.h>

//static uint8_t lcd_send_data = 0;

//extern  void HalLcd_HW_WaitUs(uint16 i);

/*
 * Initialize LCD Service
 */
void HalLcdInit(void);

//LCD Init For 1.44Inch LCD Panel with ST7735R.
void Lcd_Init(void)
{
    HAL_Delay(10);
    //开LCD背光
    HAL_GPIO_WritePin(LCD_GPIO_Port, LCD_Pin, GPIO_PIN_SET);

    Lcd_WriteIndex(0x11);
    HAL_Delay(3);//Sleep exit HAL_Delay(10);
    Lcd_WriteIndex(0x11);
//        HAL_Delay(1000);
//        HAL_Delay(1000);
//        HAL_Delay(1000);
    HAL_Delay(5);
    Lcd_WriteIndex(0x36);
    HAL_Delay(5);
    Lcd_WriteIndex(0x36);//MX, MY, RGB mode
    HAL_Delay(5);
    Lcd_WriteData(0xC8);
    HAL_Delay(5);
    Lcd_WriteData(0xC8);
    HAL_Delay(5);
    Lcd_WriteIndex(0x3A);
    HAL_Delay(5);
    Lcd_WriteIndex(0x3A);//65k mode
    HAL_Delay(5);
    Lcd_WriteData(0x05);
    HAL_Delay(5);
    Lcd_WriteData(0x05);
    HAL_Delay(20);
    Lcd_WriteIndex(0x29);
    HAL_Delay(5);
    Lcd_WriteIndex(0x29);//Display on//有问题
}


//
///*
// * Write a string to the LCD
// */
//
//void HalLcdWriteString ( char *str, uint8 option);
//
///*
// * Write a value to the LCD
// */
//extern void HalLcdWriteValue ( uint32 value, const uint8 radix, uint8 option);
//
///*
// * Write a value to the LCD
// */
//extern void HalLcdWriteScreen( char *line1, char *line2 );
//
///*
// * Write a string followed by a value to the LCD
// */
//extern void HalLcdWriteStringValue( char *title, uint16 value, uint8 format, uint8 line );
//
///*
// * Write a string followed by 2 values to the LCD
// */
//extern void HalLcdWriteStringValueValue( char *title, uint16 value1, uint8 format1, uint16 value2, uint8 format2, uint8 line );
//
///*
// * Write a percentage bar to the LCD
// */
//extern void HalLcdDisplayPercentBar( char *title, uint8 value );
//
//extern void Gui_DrawFont_GBK16(uint16 x, uint16 y, uint16 fc, uint16 bc, uint8 *s);
//void Line_Dsp_single_colour(unsigned int x_start,unsigned int y_start,unsigned int x_end,unsigned int y_end,unsigned int color);
//void dsp_single_colour(int color);


void Delay_ms(int time)
{
    int i,j;
    for(i=0; i<time*10; i++)
    {
        for(j=0; j<100; j++)
        {

        }
    }
}

void SPI_WriteData(uint8_t Data)
{
    unsigned char i;
    for(i=8; i>0; i--)
    {
        if(Data & 0x80)
        {
            LCD_SDA_SET;              //数据输出高电平
        }
        else
        {
            LCD_SDA_CLR;               //数据输出低电平
        }
        LCD_SCL_SET;              //时钟高
        LCD_SCL_CLR;              //时钟低
        Data  <<=  1;
    }
}

void LCD_WriteData_16Bit(uint16_t Data)
{
    LCD_CS_CLR;
    LCD_RS_SET;
    SPI_WriteData(Data>>8); 	//写入高8位数据
    SPI_WriteData(Data); 			//写入低8位数据
    LCD_CS_SET;
}

//向液晶屏写一个8位指令
void Lcd_WriteIndex(uint8_t Index)
{
    //SPI 写命令时序开始
    LCD_CS_CLR;
    LCD_RS_CLR;           //LCD_RS_CLR
    SPI_WriteData(Index);
    LCD_CS_SET;
}
//向液晶屏写一个8位数据
void Lcd_WriteData(uint8_t Data)
{
    LCD_CS_CLR;
    LCD_RS_SET;
    SPI_WriteData(Data);
    LCD_CS_SET;
}

void Lcd_WriteReg(uint8_t Index,uint8_t Data)
{
    LCD_CS_CLR;
    Lcd_WriteIndex(Index);
    Lcd_WriteData(Data);
    LCD_CS_SET;
}



////LCD Init For 1.44Inch LCD Panel with ST7735R.
//void Lcd_Init(void)
//{
//	Lcd_WriteIndex(0x11);//Sleep exit
//	HAL_Delay(120);
//
//	Lcd_WriteIndex(0x36); //MX, MY, RGB mode
//	Lcd_WriteData(0xC8);
//
//	Lcd_WriteIndex(0x3A); //65k mode
//	Lcd_WriteData(0x05);
//
//	Lcd_WriteIndex(0x29);//Display on
//}

/*************************************************
函数名：LCD_Set_Region
功能：设置lcd显示区域，在此区域写点数据自动换行
入口参数：xy起点和终点
返回值：无
*************************************************/
void Lcd_SetRegion(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end)
{
    Lcd_WriteIndex(0x2a);
    Lcd_WriteData(0x00);
    Lcd_WriteData(x_start+2);
    Lcd_WriteData(0x00);
    Lcd_WriteData(x_end+2);

    Lcd_WriteIndex(0x2b);
    Lcd_WriteData(0x00);
    Lcd_WriteData(y_start+3);
    Lcd_WriteData(0x00);
    Lcd_WriteData(y_end+3);

    Lcd_WriteIndex(0x2c);

}
/*************************************************
函数名：LCD_Set_XY
功能：设置lcd显示起始点
入口参数：xy坐标
返回值：无
*************************************************/
void Lcd_SetXY(uint16_t x,uint16_t y)
{
    Lcd_SetRegion(x,y,x,y);
}


/*************************************************
函数名：LCD_DrawPoint
功能：画一个点
入口参数：无
返回值：无
*************************************************/
void Gui_DrawPoint(uint16_t x,uint16_t y,uint16_t Data)
{
    Lcd_SetRegion(x,y,x+1,y+1);
    LCD_WriteData_16Bit(Data);

}

/*****************************************
 函数功能：读TFT某一点的颜色
 出口参数：color  点颜色值
******************************************/
//unsigned int Lcd_ReadPoint(uint16_t x,uint16_t y)
//{
//  unsigned int Data;
//  Lcd_SetXY(x,y);
//
//  //Lcd_ReadData();//丢掉无用字节
//  //Data=Lcd_ReadData();
//  Lcd_WriteData(Data);
//  return Data;
//}
/*************************************************
函数名：Lcd_Clear
功能：全屏清屏函数
入口参数：填充颜色COLOR
返回值：无
*************************************************/
void Lcd_Clear(uint16_t Color)
{
    unsigned int i,m;
    Lcd_SetRegion(0,0,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
    Lcd_WriteIndex(0x2C);
    for(i=0; i<X_MAX_PIXEL; i++)
        for(m=0; m<Y_MAX_PIXEL; m++)
        {
            LCD_WriteData_16Bit(Color);
        }
}
//取模方式 水平扫描 从左到右 低位在前
void showimage_farsight(const unsigned char *p) //显示128*35 QQ图片
{
    int i;
    unsigned char picH,picL;
    //Lcd_Clear(WHITE); //清屏
    Lcd_SetRegion(0,0,127,34);		//坐标设置
    for(i=0; i<128*35; i++)
    {
        picL=*(p+i*2);	//数据低位在前
        picH=*(p+i*2+1);
        LCD_WriteData_16Bit(picH<<8|picL);
    }
}

void showimage(const unsigned char *p) //显示128*35 QQ图片
{
    int i;
    unsigned char picH,picL;
    //Lcd_Clear(WHITE); //清屏
    Lcd_SetRegion(0,0,127,127);		//坐标设置
    for(i=0; i<128*128; i++)
    {
        picL=*(p+i*2);	//数据低位在前
        picH=*(p+i*2+1);
        LCD_WriteData_16Bit(picH<<8|picL);
    }
}

//void Gui_DrawFont_GBK16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s)
//{
//	unsigned char i,j;
//	unsigned short k,x0;
//	x0=x;
//
//	while(*s)
//	{
//		if((*s) < 128)
//		{
//			k=*s;
//			if (k == 13)
//			{
//				x=x0;
//				y+=16;
//			}
//			else
//			{
//				if (k>32) k-=32; else k=0;
//
//			    for(i=0;i<16;i++)
//				for(j=0;j<8;j++)
//					{
//				    	if(asc16[k*16+i]&(0x80>>j))	Gui_DrawPoint(x+j,y+i,fc);
//						else
//						{
//							if (fc!=bc) Gui_DrawPoint(x+j,y+i,bc);
//						}
//					}
//				x+=8;
//			}
//			s++;
//		}
//
//		else
//		{
//
//
//			for (k=0;k<hz16_num;k++)
//			{
//			  if ((hz16[k].Index[0]==*(s))&&(hz16[k].Index[1]==*(s+1)))
//			  {
//				    for(i=0;i<16;i++)
//				    {
//						for(j=0;j<8;j++)
//							{
//						    	if(hz16[k].Msk[i*2]&(0x80>>j))	Gui_DrawPoint(x+j,y+i,fc);
//								else {
//									if (fc!=bc) Gui_DrawPoint(x+j,y+i,bc);
//								}
//							}
//						for(j=0;j<8;j++)
//							{
//						    	if(hz16[k].Msk[i*2+1]&(0x80>>j))	Gui_DrawPoint(x+j+8,y+i,fc);
//								else
//								{
//									if (fc!=bc) Gui_DrawPoint(x+j+8,y+i,bc);
//								}
//							}
//				    }
//				}
//			  }
//			s+=2;x+=16;
//		}
//
//	}
//}
void Gui_DrawFont_GBK16(uint16_t x0, uint16_t y0, uint16_t fc, uint16_t bc, uint8_t *s)
{
    int i,j,k,x,y,xx;

    unsigned char qm;

    long int ulOffset;

    char  ywbuf[32];
// char   temp[2];

    for(i = 0; i<strlen((char*)s); i++)
    {
        if(((unsigned char)(*(s+i))) >= 161)
        {
//      temp[0] = *(s+i);
//      temp[1] = '\0';
            return;
        }

        else
        {
            qm = *(s+i);

            ulOffset = (long int)(qm) * 16;

            for (j = 0; j < 16; j ++)
            {
                ywbuf[j]=Zk_ASCII8X16[ulOffset+j];
            }

            for(y = 0; y < 16; y++)
            {
                for(x=0; x<8; x++)
                {
                    k=x % 8;

                    if(ywbuf[y]&(0x80 >> k))
                    {
                        xx=x0+x+i*8;
                        Gui_DrawPoint(xx,y+y0,fc);
                    }
                    else
                    {

                        xx=x0+x+i*8;
                        Gui_DrawPoint(xx,y+y0,bc);
                    }
                }
            }

        }
    }
}
void Gui_DrawFont_1616(uint16_t x0, uint16_t y0, uint16_t fc, uint16_t bc,const char tfont16[][32],int num)
{
	uint16_t i,j;
	uint8_t Hznum;
	for(Hznum = 0;Hznum < num;Hznum++)
	{
		// 设置汉子显示的区域
		Lcd_SetRegion(x0+Hznum*16,y0,x0+16-1+Hznum*16,y0+16-1);
		for(i = 0;i < 32;i++) //一个汉字的字模由32个元素组成
		{	
			for(j = 0;j < 8;j++) //每个元素有8位，每一位代表一个亮点
			{
				if(tfont16[Hznum][i] & (0x80 >> j))
				{
					LCD_WriteData_16Bit(fc);
				}
				else	
				{
					LCD_WriteData_16Bit(bc);
				}
			}
		}
	}
}
