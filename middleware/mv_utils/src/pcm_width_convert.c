#include <nds32_intrinsic.h>
#include "type.h"
#include "string.h"

/**********************************************
Convert pcm width type(WIDTH_L24BIT/WIDTH_24BIT/WIDTH_16BIT@ LittleEndian   WIDTH_32BIT/WIDTH_H24BIT需API内启用 )
Param:
    SrcPcm    - pcm to be converted
    SrcType   - SrcPcm's type
    DstPcm    - converted pcm buffer,can be the same as SrcPcm
    DstType   - DstPcm's type
    PcmNums   - Num(SampleNum * Channels) to be converted
Return:
    Converted pcm num, 0 means no conversion
**********************************************/
uint32_t PcmWidthConvert(void* SrcPcm, uint8_t SrcType,
                       void* DstPcm, uint8_t DstType, uint32_t PcmNums)
{
	uint32_t n;

	switch(((uint16_t)SrcType<<8) | DstType)
	{
		case ((uint16_t)WIDTH_L24BIT<<8) | WIDTH_16BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)SrcPcm;
			int16_t	*DstPcm16 = (int16_t *)DstPcm;
			for(n = 0; n < PcmNums; n++)
			{
				DstPcm16[n] = __nds32__clips((SrcPcm32[n] + 128) >> 8, (16) - 1);
			}
			break;
		}

		case ((uint16_t)WIDTH_16BIT<<8) | WIDTH_L24BIT:
		{
			int16_t	*SrcPcm16 = (int16_t *)SrcPcm;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = PcmNums; n > 0 ; n--)
			{
				DstPcm32[n - 1] = ((int32_t)(SrcPcm16[n - 1])) << 8;
			}
			break;
		}

		case ((uint16_t)WIDTH_16BIT<<8) | WIDTH_24BIT:
		{
			uint8_t	*SrcByte = (uint8_t *)SrcPcm;
			uint8_t	*DstByte = (uint8_t *)DstPcm;
			for(n = PcmNums; n > 0 ; n--)
			{
				DstByte[(n - 1) * 3 + 2] = SrcByte[(n - 1) * 2 + 1];
				DstByte[(n - 1) * 3 + 1] = SrcByte[(n - 1) * 2 + 0];
				DstByte[(n - 1) * 3 + 0] = 0;
			}
			break;
		}

		case ((uint16_t)WIDTH_24BIT<<8) | WIDTH_16BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)(SrcPcm - 1);
			int32_t Pcm32;
			int16_t	*DstPcm16 = (int16_t *)(DstPcm);

			for(n = 0; n < PcmNums; n++)
			{
				Pcm32 = SrcPcm32[n];
				DstPcm16[n] = __nds32__clips((Pcm32 + 0x8000) >> 16, (16) - 1);
			}
			break;
		}

		case ((uint16_t)WIDTH_24BIT<<8) | WIDTH_L24BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)(SrcPcm - 1);//符号位保留
			int32_t Pcm32;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = PcmNums; n > 0 ; n--)
			{
				Pcm32 = SrcPcm32[n - 1];
				DstPcm32[n - 1] = Pcm32>>8;
			}
			//对照：
//			uint32_t	*Dst32 = (uint32_t *)DstPcm;
//			uint32_t	*Src32 = (uint32_t *)(SrcPcm);
//			for(n = PcmNums; n > 0 ; n--)
//			{					//WIDTH_24ToL24(Src32[n - 1]);
//				Dst32[n - 1] = ((Src32[n - 1]) & 0xFFFFFF) | ((Src32[n - 1] & 0x800000)? 0xFF000000 : 0);
//			}
			break;
		}

		case ((uint16_t)WIDTH_L24BIT<<8) | WIDTH_24BIT:
		{
			uint8_t	*DstByte = (uint8_t *)DstPcm;
			uint8_t	*SrcByte = (uint8_t *)SrcPcm;
			for(n = 0; n < PcmNums; n++)
			{
				DstByte[n * 3 + 2] = SrcByte[n * 4 + 2];
				DstByte[n * 3 + 1] = SrcByte[n * 4 + 1];
				DstByte[n * 3 + 0] = SrcByte[n * 4 + 0];
			}
			//对照
//				for(s = 0; s<RealLen*channel; s++)
//				{
//					memcpy(&DstByte[s*3], &SrcByte[s*4], 3);
//				}
			break;
		}

#if 0//+ WIDTH_H24BIT
		case ((uint16_t)WIDTH_H24BIT<<8) | WIDTH_16BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)SrcPcm;
			int16_t	*DstPcm16 = (int16_t *)DstPcm;
			for(n = 0; n < PcmNums; n++)
			{
				DstPcm16[n] = __nds32__clips((SrcPcm32[n] + 0x8000) >> 8, (16) - 1);
			}
			break;
		}

		case ((uint16_t)WIDTH_16BIT<<8) | WIDTH_H24BIT:
		{
			int16_t	*SrcPcm16 = (int16_t *)SrcPcm;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = PcmNums; n > 0 ; n--)
			{
				DstPcm32[n - 1] = ((int32_t)(SrcPcm16[n - 1])) << 16;
			}
			break;
		}

		case ((uint16_t)WIDTH_24BIT<<8) | WIDTH_H24BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)(SrcPcm - 1);//符号位保留
			int32_t Pcm32;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = PcmNums; n > 0 ; n--)
			{
				Pcm32 = SrcPcm32[n - 1];
				DstPcm32[n - 1] = Pcm32 & 0xffffff00;;
			}
			break;
		}

		case ((uint16_t)WIDTH_H24BIT<<8) | WIDTH_24BIT:
		{
			uint8_t	*DstByte = (uint8_t *)DstPcm;
			uint8_t	*SrcByte = (uint8_t *)SrcPcm;
			for(n = 0; n < PcmNums; n++)
			{
				DstByte[n * 3 + 2] = SrcByte[n * 4 + 3];
				DstByte[n * 3 + 1] = SrcByte[n * 4 + 2];
				DstByte[n * 3 + 0] = SrcByte[n * 4 + 1];
			}
			break;
		}

		case ((uint16_t)WIDTH_L24BIT<<8) | WIDTH_H24BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)SrcPcm;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = 0; n < PcmNums ; n++)
			{
				DstPcm32[n] = SrcPcm32[n]<<8;
			}
			break;
		}

		case ((uint16_t)WIDTH_H24BIT<<8) | WIDTH_L24BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)SrcPcm;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = 0; n < PcmNums ; n++)
			{
				DstPcm32[n] = SrcPcm32[n]>>8;
			}
			break;
		}
#endif //+ WIDTH_H24BIT
#if 0//+ WIDTH_32BIT
		case ((uint16_t)WIDTH_32BIT<<8) | WIDTH_16BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)SrcPcm;
			int16_t	*DstPcm16 = (int16_t *)DstPcm;
			for(n = 0; n < PcmNums; n++)
			{
				DstPcm16[n] = __nds32__clips((SrcPcm32[n] + 0x8000) >> 8, (16) - 1);
			}
			break;
		}

		case ((uint16_t)WIDTH_16BIT<<8) | WIDTH_32BIT:
		{
			int16_t	*SrcPcm16 = (int16_t *)SrcPcm;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = PcmNums; n > 0 ; n--)
			{
				DstPcm32[n - 1] = ((int32_t)(SrcPcm16[n - 1])) << 16;
			}
			break;
		}

		case ((uint16_t)WIDTH_24BIT<<8) | WIDTH_32BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)(SrcPcm - 1);//符号位保留
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			int32_t Pcm32;
			for(n = PcmNums; n > 0 ; n--)
			{
				Pcm32 = SrcPcm32[n - 1];
				DstPcm32[n - 1] = Pcm32 & 0xffffff00;
			}
			break;
		}

		case ((uint16_t)WIDTH_32BIT<<8) | WIDTH_24BIT:
		{
			uint8_t	*DstByte = (uint8_t *)DstPcm;
			int32_t	*Src32 = (int32_t *)SrcPcm;
			int32_t Pcm32;
			uint8_t *PcmByte = (uint8_t*)&Pcm32;
			for(n = 0; n < PcmNums; n++)
			{
				Pcm32 = Src32[n] + 0x80;
				DstByte[n * 3 + 2] = PcmByte[3];
				DstByte[n * 3 + 1] = PcmByte[2];
				DstByte[n * 3 + 0] = PcmByte[1];
			}
			break;
		}

		case ((uint16_t)WIDTH_L24BIT<<8) | WIDTH_32BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)SrcPcm;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = 0; n < PcmNums ; n++)
			{
				DstPcm32[n] = SrcPcm32[n]<<8;
			}
			break;
		}

		case ((uint16_t)WIDTH_32BIT<<8) | WIDTH_L24BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)SrcPcm;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = 0; n < PcmNums ; n++)
			{
				DstPcm32[n] = __nds32__clips((SrcPcm32[n] + 0x80) >> 8, (32) - 1);
			}
			break;
		}

		case ((uint16_t)WIDTH_32BIT<<8) | WIDTH_H24BIT:
		{
			int32_t	*SrcPcm32 = (int32_t *)SrcPcm;
			int32_t	*DstPcm32 = (int32_t *)DstPcm;
			for(n = 0; n < PcmNums ; n++)
			{
				DstPcm32[n] = __nds32__clips((SrcPcm32[n] + 0x80), (32) - 1);
			}
			break;
		}
#endif //+ WIDTH_32BIT
		case ((uint16_t)WIDTH_H24BIT<<8) | WIDTH_32BIT:
			SrcType = DstType;
		default:
			if(SrcType == DstType)
			{
				if(SrcPcm != DstPcm)
				{
					memcpy(DstPcm, SrcPcm, PcmNums * PCM_BYTE(SrcType));
				}
				break;
			}
			return 0;
	}
	return PcmNums;
}
