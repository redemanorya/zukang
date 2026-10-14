/*
 * i2s2_mix.c
 *
 *  Created on: 2026年4月22日
 *      Author: mike
 */

#include <string.h>
#include "type.h"
#include "dma.h"
#include "reset.h"
#include "chip_interrupt.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "app_config.h"
#include "mode_task_api.h"
#include "main_task.h"
#include "debug.h"
#include "i2s_interface.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "effect_service.h"
#include "ctrlvars.h"
#include "audio_effect_control.h"
#include "i2s_dpll_policy.h"
#if defined(CFG_I2S2_MIX_PLURAL_IN_FIFO_EN) || defined(CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN)
#include "mcu_circular_buf.h"
#endif

#if defined(CFG_I2S2_MIX_IN_EN) && ((I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN))
#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
	static uint8_t *I2S2_IN_LineFifo[8] = {0};
	static MCU_CIRCULAR_CONTEXT I2S2_IN_LineCircularBuf[8];
	static uint16_t I2S2_IN_Switch_FrameSize = 0;
#endif
	uint32_t * I2S2_IN_Switch_Buf = NULL;
	uint8_t I2S2INChNum = 0;
	static uint8_t I2S2_IN_LineMask = 0xFF;  // 输入线使能掩码, bit[a]=1表示第a条线有效
	#define I2S2MIXPLURALCH0_SOURCE_NUM					I2SPLURALCH0_SOURCE_NUM
	#define I2S2MIXPLURALCH1_SOURCE_NUM					I2SPLURALCH1_SOURCE_NUM
	#define I2S2MIXPLURALCH2_SOURCE_NUM					I2SPLURALCH2_SOURCE_NUM
	#define I2S2MIXPLURALCH3_SOURCE_NUM					I2SPLURALCH3_SOURCE_NUM
	#define I2S2MIXPLURALCH4_SOURCE_NUM					I2SPLURALCH4_SOURCE_NUM
	#define I2S2MIXPLURALCH5_SOURCE_NUM					I2SPLURALCH5_SOURCE_NUM
	#define I2S2MIXPLURALCH6_SOURCE_NUM					I2SPLURALCH6_SOURCE_NUM
	#define I2S2MIXPLURALCH7_SOURCE_NUM					I2SPLURALCH7_SOURCE_NUM
#endif
#if defined(CFG_I2S2_MIX_OUT_EN) && ((I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN))
#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
	static uint8_t *I2S2_OUT_LineFifo[8] = {0};
	static MCU_CIRCULAR_CONTEXT I2S2_OUT_LineCircularBuf[8];
	static uint16_t I2S2_OUT_Switch_FrameSize = 0;
#endif
	uint32_t * I2S2_OUT_Switch_Buf = NULL;
	uint8_t I2S2OUTChNum = 0;
	static uint8_t I2S2_OUT_LineMask = 0xFF;  // 输出线使能掩码, bit[a]=1表示第a条线有效
	#define I2S2MIXPLURALCH0_SINK_NUM					I2SPLURALCH0_SINK_NUM
	#define I2S2MIXPLURALCH1_SINK_NUM					I2SPLURALCH1_SINK_NUM
	#define I2S2MIXPLURALCH2_SINK_NUM					I2SPLURALCH2_SINK_NUM
	#define I2S2MIXPLURALCH3_SINK_NUM					I2SPLURALCH3_SINK_NUM
	#define I2S2MIXPLURALCH4_SINK_NUM					I2SPLURALCH4_SINK_NUM
	#define I2S2MIXPLURALCH5_SINK_NUM					I2SPLURALCH5_SINK_NUM
	#define I2S2MIXPLURALCH6_SINK_NUM					I2SPLURALCH6_SINK_NUM
	#define I2S2MIXPLURALCH7_SINK_NUM					I2SPLURALCH7_SINK_NUM
#endif

#ifdef CFG_FUNC_I2S2_MIX
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
static bool sI2S2MixStreamsEnabled = FALSE;
#endif

#if defined(CFG_I2S2_MIX_IN_EN) && ((I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN))
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
	static uint16_t I2S2MixInPluralLineDataByte(void)
	{
		return sizeof(PCM_DATA_TYPE) * 2;
	}

	static void I2S2MixInPluralFifoReset(void)
	{
		uint8_t InSrcCnt = I2S2INChNum / 2;
		for(uint8_t i = 0; i < InSrcCnt; i++)
		{
			I2S2_IN_LineCircularBuf[i].R = 0;
			I2S2_IN_LineCircularBuf[i].W = 0;
		}
	}

	static void I2S2MixInPluralPump(void)
	{
		uint8_t InSrcCnt = I2S2INChNum / 2;
		uint32_t DmaBytes;
		uint32_t MinSpaceBytes = 0xFFFFFFFF;
		uint32_t FramesToPump;
		PCM_DATA_TYPE *pInput;

		if(I2S2_IN_Switch_Buf == NULL || InSrcCnt == 0 || I2S2INChNum == 0)
		{
			return;
		}

		for(uint8_t i = 0; i < InSrcCnt; i++)
		{
			if(!(I2S2_IN_LineMask & (1 << i)))
			{
				continue;
			}

			uint32_t SpaceBytes = MCUCircular_GetSpaceLen(&I2S2_IN_LineCircularBuf[i]);
			if(SpaceBytes < MinSpaceBytes)
			{
				MinSpaceBytes = SpaceBytes;
			}
		}

		if(MinSpaceBytes == 0xFFFFFFFF || MinSpaceBytes < I2S2MixInPluralLineDataByte())
		{
			return;
		}

		DmaBytes = DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX);
		FramesToPump = DmaBytes / (sizeof(PCM_DATA_TYPE) * I2S2INChNum);
		if(FramesToPump == 0)
		{
			return;
		}

		if(FramesToPump > (MinSpaceBytes / I2S2MixInPluralLineDataByte()))
		{
			FramesToPump = MinSpaceBytes / I2S2MixInPluralLineDataByte();
		}

		if(FramesToPump > I2S2_IN_Switch_FrameSize)
		{
			FramesToPump = I2S2_IN_Switch_FrameSize;
		}

		if(FramesToPump == 0)
		{
			return;
		}

		DMA_CircularDataGet(PERIPHERAL_1_ID_I2S2_RX, I2S2_IN_Switch_Buf, FramesToPump * sizeof(PCM_DATA_TYPE) * I2S2INChNum);

		pInput = (PCM_DATA_TYPE *)I2S2_IN_Switch_Buf;
		for(uint16_t frame = 0; frame < FramesToPump; frame++)
		{
			for(uint8_t line = 0; line < InSrcCnt; line++)
			{
				if(I2S2_IN_LineMask & (1 << line))
				{
					MCUCircular_PutData(&I2S2_IN_LineCircularBuf[line],&pInput[frame * I2S2INChNum + line * 2],I2S2MixInPluralLineDataByte());
				}
			}
		}
	}

	static uint16_t I2S2MixInPluralLineDataLenGet(uint8_t LineIndex)
	{
		if(LineIndex >= (I2S2INChNum / 2) || !(I2S2_IN_LineMask & (1 << LineIndex)))
		{
			return 0;
		}

		I2S2MixInPluralPump();
		return MCUCircular_GetDataLen(&I2S2_IN_LineCircularBuf[LineIndex]) / I2S2MixInPluralLineDataByte();
	}

	static uint16_t I2S2MixInPluralLineDataGet(uint8_t LineIndex, void *pcm_out, uint16_t MaxPoint)
	{
		uint32_t CopyBytes;

		if(pcm_out == NULL || MaxPoint == 0)
		{
			return 0;
		}

		memset(pcm_out, 0, MaxPoint * I2S2MixInPluralLineDataByte());

		if(LineIndex >= (I2S2INChNum / 2) || !(I2S2_IN_LineMask & (1 << LineIndex)))
		{
			return 0;
		}

		I2S2MixInPluralPump();
		CopyBytes = MCUCircular_GetDataLen(&I2S2_IN_LineCircularBuf[LineIndex]);
		if(CopyBytes > (uint32_t)MaxPoint * I2S2MixInPluralLineDataByte())
		{
			CopyBytes = (uint32_t)MaxPoint * I2S2MixInPluralLineDataByte();
		}

		if(CopyBytes)
		{
			MCUCircular_GetData(&I2S2_IN_LineCircularBuf[LineIndex], pcm_out, CopyBytes);
		}

		return CopyBytes / I2S2MixInPluralLineDataByte();
	}
	#endif

	//sample为单位
	uint16_t AudioI2S2_PluralCH0_DataLenGet(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataLenGet(0);
	#else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX)/(I2S2INChNum/2);
	#endif
	}

	//sample 为单位，buf 大小：8 * MaxSize
	uint16_t AudioI2S2_PluralCH0_DataGet(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataGet(0, pcm_out, MaxPoint);
	#else
		AudioI2SDataRXCarry(I2S2_MODULE, I2S2INChNum, I2S2_IN_LineMask, I2S2_IN_Switch_Buf, \
				AudioCoreFrameSizeGet(AudioCoreGetNetBySourceChannelNUM(I2S2MIXPLURALCH0_SOURCE_NUM)));   //为了减少Memory，全部一次处理好
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH1_DataLenGet(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataLenGet(1);
	#else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX)/(I2S2INChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH1_DataGet(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataGet(1, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_DataGet 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH2_DataLenGet(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataLenGet(2);
	#else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX)/(I2S2INChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH2_DataGet(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataGet(2, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_DataGet 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH3_DataLenGet(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataLenGet(3);
	#else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX)/(I2S2INChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH3_DataGet(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataGet(3, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_DataGet 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH4_DataLenGet(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataLenGet(4);
	#else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX)/(I2S2INChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH4_DataGet(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataGet(4, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_DataGet 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH5_DataLenGet(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataLenGet(5);
	#else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX)/(I2S2INChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH5_DataGet(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataGet(5, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_DataGet 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH6_DataLenGet(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataLenGet(6);
	#else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX)/(I2S2INChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH6_DataGet(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataGet(6, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_DataGet 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH7_DataLenGet(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataLenGet(7);
	#else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX)/(I2S2INChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH7_DataGet(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		return I2S2MixInPluralLineDataGet(7, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_DataGet 已经做完
		return 0;
	#endif
	}
#endif

#if defined(CFG_I2S2_MIX_OUT_EN) && ((I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN))
#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
	uint16_t I2S2MixOutPluralLineDataByte(void)
	{
		return sizeof(PCM_DATA_TYPE) * 2;
	}

	static void I2S2MixOutPluralFifoReset(void)
	{
		uint8_t OutSinkCnt = I2S2OUTChNum / 2;
		for(uint8_t i = 0; i < OutSinkCnt; i++)
		{
			I2S2_OUT_LineCircularBuf[i].R = 0;
			I2S2_OUT_LineCircularBuf[i].W = 0;
		}
	}

	static void I2S2MixOutPluralPump(void)
	{
		uint8_t OutSinkCnt = I2S2OUTChNum / 2;
		uint32_t MinDataBytes = 0xFFFFFFFF;
		uint32_t SpaceFrames;
		uint32_t FramesToPush;
		PCM_DATA_TYPE *pOutput;
		PCM_DATA_TYPE SampleLR[2];

		if(I2S2_OUT_Switch_Buf == NULL || OutSinkCnt == 0 || I2S2OUTChNum == 0)
		{
			return;
		}

		for(uint8_t i = 0; i < OutSinkCnt; i++)
		{
			if(!(I2S2_OUT_LineMask & (1 << i)))
			{
				continue;
			}

			uint32_t DataBytes = MCUCircular_GetDataLen(&I2S2_OUT_LineCircularBuf[i]);
			if(DataBytes < MinDataBytes)
			{
				MinDataBytes = DataBytes;
			}
		}

		if(MinDataBytes == 0xFFFFFFFF || MinDataBytes < I2S2MixOutPluralLineDataByte())
		{
			return;
		}

		SpaceFrames = DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX) / (sizeof(PCM_DATA_TYPE) * I2S2OUTChNum);
		FramesToPush = MinDataBytes / I2S2MixOutPluralLineDataByte();

		if(FramesToPush > SpaceFrames)
		{
			FramesToPush = SpaceFrames;
		}

		if(FramesToPush > I2S2_OUT_Switch_FrameSize)
		{
			FramesToPush = I2S2_OUT_Switch_FrameSize;
		}

		if(FramesToPush == 0)
		{
			return;
		}

		memset(I2S2_OUT_Switch_Buf, 0, FramesToPush * sizeof(PCM_DATA_TYPE) * I2S2OUTChNum);
		pOutput = (PCM_DATA_TYPE *)I2S2_OUT_Switch_Buf;
		for(uint16_t frame = 0; frame < FramesToPush; frame++)
		{
			for(uint8_t line = 0; line < OutSinkCnt; line++)
			{
				if(I2S2_OUT_LineMask & (1 << line))
				{
					MCUCircular_GetData(&I2S2_OUT_LineCircularBuf[line], SampleLR, I2S2MixOutPluralLineDataByte());
					pOutput[frame * I2S2OUTChNum + line * 2] = SampleLR[0];
					pOutput[frame * I2S2OUTChNum + line * 2 + 1] = SampleLR[1];
				}
			}
		}

		DMA_CircularDataPut(PERIPHERAL_1_ID_I2S2_TX, I2S2_OUT_Switch_Buf, FramesToPush * sizeof(PCM_DATA_TYPE) * I2S2OUTChNum);
	}

	static uint16_t I2S2MixOutPluralLineSpaceLenGet(uint8_t LineIndex)
	{
		if(LineIndex >= (I2S2OUTChNum / 2) || !(I2S2_OUT_LineMask & (1 << LineIndex)))
		{
			return 0;
		}

		I2S2MixOutPluralPump();
		return MCUCircular_GetSpaceLen(&I2S2_OUT_LineCircularBuf[LineIndex]) / I2S2MixOutPluralLineDataByte();
	}

	static uint16_t I2S2MixOutPluralLinePutData(uint8_t LineIndex, void *pcm_out, uint16_t MaxPoint)
	{
		uint32_t CopyBytes;

		if(pcm_out == NULL || MaxPoint == 0)
		{
			return 0;
		}

            if(LineIndex >= (I2S2OUTChNum / 2) || !(I2S2_OUT_LineMask & (1 << LineIndex)))
		{
			return 0;
		}

		I2S2MixOutPluralPump();

		CopyBytes = MCUCircular_GetSpaceLen(&I2S2_OUT_LineCircularBuf[LineIndex]);
		if(CopyBytes > (uint32_t)MaxPoint * I2S2MixOutPluralLineDataByte())
		{
			CopyBytes = (uint32_t)MaxPoint * I2S2MixOutPluralLineDataByte();
		}

		if(CopyBytes)
		{
			MCUCircular_PutData(&I2S2_OUT_LineCircularBuf[LineIndex], pcm_out, CopyBytes);
		}

		I2S2MixOutPluralPump();
		return CopyBytes / I2S2MixOutPluralLineDataByte();
	}
#endif
	//sample为单位
	uint16_t AudioI2S2_PluralCH0_GetSpaceLen(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLineSpaceLenGet(0);
	#else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX)/(I2S2OUTChNum/2);
	#endif
	}

	//sample 为单位，buf 大小：8 * MaxSize
	uint16_t AudioI2S2_PluralCH0_PutData(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLinePutData(0, pcm_out, MaxPoint);
	#else
		AudioI2SDataTXCarry(I2S2_MODULE, I2S2OUTChNum, I2S2_OUT_LineMask, I2S2_OUT_Switch_Buf, \
				AudioCoreFrameSizeGet(AudioCoreGetNetBySinkChannelNUM(I2S2MIXPLURALCH0_SINK_NUM)));   //为了减少Memory，全部一次处理好
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH1_GetSpaceLen(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLineSpaceLenGet(1);
	#else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX)/(I2S2OUTChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH1_PutData(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLinePutData(1, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_PutData 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH2_GetSpaceLen(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLineSpaceLenGet(2);
	#else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX)/(I2S2OUTChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH2_PutData(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLinePutData(2, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_PutData 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH3_GetSpaceLen(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLineSpaceLenGet(3);
	#else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX)/(I2S2OUTChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH3_PutData(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLinePutData(3, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_PutData 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH4_GetSpaceLen(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLineSpaceLenGet(4);
	#else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX)/(I2S2OUTChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH4_PutData(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLinePutData(4, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_PutData 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH5_GetSpaceLen(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLineSpaceLenGet(5);
	#else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX)/(I2S2OUTChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH5_PutData(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLinePutData(5, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_PutData 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH6_GetSpaceLen(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLineSpaceLenGet(6);
	#else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX)/(I2S2OUTChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH6_PutData(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLinePutData(6, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_PutData 已经做完
		return 0;
	#endif
	}

	uint16_t AudioI2S2_PluralCH7_GetSpaceLen(void)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLineSpaceLenGet(7);
	#else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX)/(I2S2OUTChNum/2);
	#endif
	}

	uint16_t AudioI2S2_PluralCH7_PutData(void *pcm_out, uint16_t MaxPoint)
	{
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		return I2S2MixOutPluralLinePutData(7, pcm_out, MaxPoint);
	#else
		//AudioI2S2_PluralCH0_PutData 已经做完
		return 0;
	#endif
	}
#endif

static void I2s2MixHardParamSet(I2SParamCt* i2s_set)
{
	// LineMode: 线数(1/2/4) -> 枚举(0/1/2), 取 IN/OUT 中较大值(应一致，否则 OUT 优先)
	uint8_t line = AUDIO_LINE_SINGLE;  //默认单线
#if defined(CFG_I2S2_MIX_OUT_EN)
	line = I2S2_MIX_OUT_LINE_CNT;
#elif defined(CFG_I2S2_MIX_IN_EN)
	line = I2S2_MIX_IN_LINE_CNT;
#endif
	i2s_set->LineMode = (line == AUDIO_LINE_QUAD) ? Quad_Mode :
	                    (line == AUDIO_LINE_DUAL) ? Dual_Mode : Single_Mode;

#ifdef CFG_FUNC_I2S2_TDM_MODE_EN
	i2s_set->TdmChNum = TDM2CHNUM;
	// TDM 不支持 Quad_Mode, 强制降为 Dual
	if (i2s_set->LineMode == Quad_Mode)
		i2s_set->LineMode = Dual_Mode;
#endif
	i2s_set->IsMasterMode = CFG_I2S2_MIX_MODE;// 0:master 1:slave
	i2s_set->SampleRate = CFG_I2S2_MIX_SAMPLERATE; //根据外设选
#ifdef CFG_FUNC_I2S2_TDM_MODE_EN
	i2s_set->I2sFormat = I2S_FORMAT_TDM;
#else
	i2s_set->I2sFormat = I2S_FORMAT_I2S;
#endif
	i2s_set->ALIGN_MODE   = I2S_LOW_BITS_ACTIVE;

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	i2s_set->I2sBits = I2S_LENGTH_24BITS;
#else
	i2s_set->I2sBits = I2S_LENGTH_16BITS;
#endif

#if defined (CFG_I2S2_MIX_IN_EN) && defined (CFG_I2S2_MIX_OUT_EN)
	i2s_set->I2sTxRxEnable = 1;
	if(GetSystemMode() != ModeBtHfPlay)  // 通话模式不支持音源mix
	{
		i2s_set->RxPeripheralID = PERIPHERAL_1_ID_I2S2_RX;
		i2s_set->RxBuf = (void*)mainAppCt.I2S2_MIX_RX_FIFO;
		i2s_set->RxLen = mainAppCt.I2S2_MIX_RX_FIFO_LEN;
	}
	i2s_set->TxPeripheralID = PERIPHERAL_1_ID_I2S2_TX;
	i2s_set->TxBuf = (void*)mainAppCt.I2S2_MIX_TX_FIFO;
	i2s_set->TxLen = mainAppCt.I2S2_MIX_TX_FIFO_LEN;
#elif defined (CFG_I2S2_MIX_IN_EN)
	i2s_set->I2sTxRxEnable = 2;
	i2s_set->RxPeripheralID = PERIPHERAL_1_ID_I2S2_RX;
	i2s_set->RxBuf = (void*)mainAppCt.I2S2_MIX_RX_FIFO;
	i2s_set->RxLen = mainAppCt.I2S2_MIX_RX_FIFO_LEN;
#elif defined (CFG_I2S2_MIX_OUT_EN)
	i2s_set->I2sTxRxEnable = 1;
	i2s_set->TxPeripheralID = PERIPHERAL_1_ID_I2S2_TX;
	i2s_set->TxBuf = (void*)mainAppCt.I2S2_MIX_TX_FIFO;
	i2s_set->TxLen = mainAppCt.I2S2_MIX_TX_FIFO_LEN;
#else
	i2s_set->I2sTxRxEnable = 0;
#endif

	SET_I2S_PIN_MODE((I2S2_MIX_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S2_MIX_LRCLK_GPIO));//lrclk
	SET_I2S_PIN_MODE((I2S2_MIX_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_BCLK_GPIO));//bclk
#ifdef CFG_I2S2_MIX_IN_EN
	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
		#if (I2S2_MIX_IN_LINE_CNT >= AUDIO_LINE_DUAL)
			#ifdef I2S2_MIX_DIN0_GPIO
				SET_I2S_PIN_MODE((I2S2_MIX_DIN0_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DIN0_GPIO));   // i2s2 din0
			#endif
			#ifdef I2S2_MIX_DIN1_GPIO
				SET_I2S_PIN_MODE((I2S2_MIX_DIN1_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DIN1_GPIO));   // i2s2 din1
			#endif
		#else
			// TDM单线: 多线宏未定义，fallback用单线DIN
			SET_I2S_PIN_MODE((I2S2_MIX_DIN_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DIN_GPIO));
		#endif
		#if (I2S2_MIX_IN_LINE_CNT >= AUDIO_LINE_QUAD)
			#ifdef I2S2_MIX_DIN2_GPIO
				SET_I2S_PIN_MODE((I2S2_MIX_DIN2_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DIN2_GPIO));   // i2s2 din2
			#endif
			#ifdef I2S2_MIX_DIN3_GPIO
				SET_I2S_PIN_MODE((I2S2_MIX_DIN3_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DIN3_GPIO));   // i2s2 din3
			#endif
		#endif
	#else
		SET_I2S_PIN_MODE((I2S2_MIX_DIN_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DIN_GPIO));//di
	#endif
#endif
#ifdef CFG_I2S2_MIX_OUT_EN
	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
		#if (I2S2_MIX_OUT_LINE_CNT >= AUDIO_LINE_DUAL)
			#ifdef I2S2_MIX_DOUT0_GPIO
				SET_I2S_PIN_MODE((I2S2_MIX_DOUT0_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DOUT0_GPIO));   // i2s2 dout0
			#endif
			#ifdef I2S2_MIX_DOUT1_GPIO
				SET_I2S_PIN_MODE((I2S2_MIX_DOUT1_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DOUT1_GPIO));   // i2s2 dout1
			#endif
		#else
			// TDM单线: 多线宏未定义，fallback用单线DOUT
			SET_I2S_PIN_MODE((I2S2_MIX_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DOUT_GPIO));
		#endif
		#if (I2S2_MIX_OUT_LINE_CNT >= AUDIO_LINE_QUAD)
			#ifdef I2S2_MIX_DOUT2_GPIO
				SET_I2S_PIN_MODE((I2S2_MIX_DOUT2_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DOUT2_GPIO));   // i2s2 dout2
			#endif
			#ifdef I2S2_MIX_DOUT3_GPIO
				SET_I2S_PIN_MODE((I2S2_MIX_DOUT3_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DOUT3_GPIO));   // i2s2 dout3
			#endif
		#endif
	#else
		SET_I2S_PIN_MODE((I2S2_MIX_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S2_MIX_DOUT_GPIO));//do
	#endif
#endif

	// 根据GPIO宏是否存在自动计算 LineMask（编译时常量，无运行时开销）
#ifdef CFG_I2S2_MIX_OUT_EN
	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	{
		I2S2_OUT_LineMask = 0;
		#ifdef CFG_FUNC_I2S2_TDM_MODE_EN
			// TDM: 通道数由TDM2CHNUM决定，不依赖物理线数
			// 如TDM8: 单线承载8ch=4个立体声对，LineMask=0x0F
			I2S2_OUT_LineMask = (1 << (I2S2_MIX_OUT_CHNUM / 2)) - 1;
		#else
			// I2S: 每线固定2ch，根据GPIO宏决定哪些线参与
			#if I2S2_MIX_OUT_LINE_CNT >= 1
			#ifdef I2S2_MIX_DOUT0_GPIO
			I2S2_OUT_LineMask |= 0x01;
			#endif
			#endif
			#if I2S2_MIX_OUT_LINE_CNT >= 2
			#ifdef I2S2_MIX_DOUT1_GPIO
			I2S2_OUT_LineMask |= 0x02;
			#endif
			#endif
			#if I2S2_MIX_OUT_LINE_CNT >= 3
			#ifdef I2S2_MIX_DOUT2_GPIO
			I2S2_OUT_LineMask |= 0x04;
			#endif
			#endif
			#if I2S2_MIX_OUT_LINE_CNT >= 4
			#ifdef I2S2_MIX_DOUT3_GPIO
			I2S2_OUT_LineMask |= 0x08;
			#endif
			#endif
		#endif
	}
	#endif
#endif
#ifdef CFG_I2S2_MIX_IN_EN
	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	{
		I2S2_IN_LineMask = 0;
		#ifdef CFG_FUNC_I2S2_TDM_MODE_EN
			// TDM: 通道数由TDM2CHNUM决定，不依赖物理线数
			I2S2_IN_LineMask = (1 << (I2S2_MIX_IN_CHNUM / 2)) - 1;
		#else
			// I2S: 每线固定2ch，根据GPIO宏决定哪些线参与
			#if I2S2_MIX_IN_LINE_CNT >= 1
			#ifdef I2S2_MIX_DIN0_GPIO
			I2S2_IN_LineMask |= 0x01;
			#endif
			#endif
			#if I2S2_MIX_IN_LINE_CNT >= 2
			#ifdef I2S2_MIX_DIN1_GPIO
			I2S2_IN_LineMask |= 0x02;
			#endif
			#endif
			#if I2S2_MIX_IN_LINE_CNT >= 3
			#ifdef I2S2_MIX_DIN2_GPIO
			I2S2_IN_LineMask |= 0x04;
			#endif
			#endif
			#if I2S2_MIX_IN_LINE_CNT >= 4
			#ifdef I2S2_MIX_DIN3_GPIO
			I2S2_IN_LineMask |= 0x08;
			#endif
			#endif
		#endif
	}
	#endif
#endif

	AudioI2S_Init(I2S2_MODULE, i2s_set);

#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
	Clock_AudioMclkSel(AUDIO_I2S2, gCtrlVars.HwCt.I2S2Ct.CLK_Source > 2 ? (gCtrlVars.HwCt.I2S2Ct.CLK_Source - 1):gCtrlVars.HwCt.I2S2Ct.CLK_Source);
#else
	gCtrlVars.HwCt.I2S2Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_I2S2);
#endif
}

#ifdef CFG_I2S2_MIX_IN_EN
static bool I2s2MixInMalloc(uint16_t FrameSize)
{
	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	if(I2S2_IN_Switch_Buf == NULL)
	{
		I2S2_IN_Switch_Buf = (uint32_t *)osPortMalloc(FrameSize * sizeof(PCM_DATA_TYPE) * I2S2_MIX_IN_CHNUM);
		if (I2S2_IN_Switch_Buf == NULL)
		{
			return FALSE;
		}
		memset(I2S2_IN_Switch_Buf, 0, FrameSize * sizeof(PCM_DATA_TYPE) * I2S2_MIX_IN_CHNUM);
	}
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
    I2S2_IN_Switch_FrameSize = FrameSize;
    for(uint8_t i = 0; i < (I2S2INChNum / 2); i++)
    {
		uint32_t FifoLen = FrameSize * sizeof(PCM_DATA_TYPE) * 2 * 3;
		if(I2S2_IN_LineFifo[i] == NULL)
		{
			I2S2_IN_LineFifo[i] = (uint8_t *)osPortMalloc(FifoLen);
			if(I2S2_IN_LineFifo[i] == NULL)
			{
				return FALSE;
			}
			memset(I2S2_IN_LineFifo[i], 0, FifoLen);
			MCUCircular_Config(&I2S2_IN_LineCircularBuf[i], I2S2_IN_LineFifo[i], FifoLen);
		}
		else
		{
			I2S2_IN_LineCircularBuf[i].R = 0;
			I2S2_IN_LineCircularBuf[i].W = 0;
		}
    }
	#endif
	#endif
	return TRUE;
}
#endif

#ifdef CFG_I2S2_MIX_OUT_EN
static bool I2s2MixOutMalloc(uint16_t FrameSize)
{
	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	if(I2S2_OUT_Switch_Buf == NULL)
	{
		I2S2_OUT_Switch_Buf = (uint32_t *)osPortMalloc(FrameSize * sizeof(PCM_DATA_TYPE) * I2S2_MIX_OUT_CHNUM);
		if (I2S2_OUT_Switch_Buf == NULL)
		{
			return FALSE;
		}
		memset(I2S2_OUT_Switch_Buf, 0, FrameSize * sizeof(PCM_DATA_TYPE) * I2S2_MIX_OUT_CHNUM);
	}
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
    I2S2_OUT_Switch_FrameSize = FrameSize;
    for(uint8_t i = 0; i < (I2S2OUTChNum / 2); i++)
    {
		uint32_t FifoLen = FrameSize * sizeof(PCM_DATA_TYPE) * 2 * 3;
		if(I2S2_OUT_LineFifo[i] == NULL)
		{
			I2S2_OUT_LineFifo[i] = (uint8_t *)osPortMalloc(FifoLen);
			if(I2S2_OUT_LineFifo[i] == NULL)
			{
				return FALSE;
			}
			memset(I2S2_OUT_LineFifo[i], 0, FifoLen);
			MCUCircular_Config(&I2S2_OUT_LineCircularBuf[i], I2S2_OUT_LineFifo[i], FifoLen);
		}
		else
		{
			I2S2_OUT_LineCircularBuf[i].R = 0;
			I2S2_OUT_LineCircularBuf[i].W = 0;
		}
    }
	#endif
	#endif
	return TRUE;
}
#endif

#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
static void I2S2MixStreamsEnable(void)
{
	if(sI2S2MixStreamsEnabled)
	{
		return;
	}
#ifdef CFG_I2S2_MIX_IN_EN
	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	{
		const uint8_t InSourceNumMap[] = {
			I2S2MIXPLURALCH0_SOURCE_NUM, I2S2MIXPLURALCH1_SOURCE_NUM,
			I2S2MIXPLURALCH2_SOURCE_NUM, I2S2MIXPLURALCH3_SOURCE_NUM,
			I2S2MIXPLURALCH4_SOURCE_NUM, I2S2MIXPLURALCH5_SOURCE_NUM,
			I2S2MIXPLURALCH6_SOURCE_NUM, I2S2MIXPLURALCH7_SOURCE_NUM
		};
		uint8_t InSrcCnt = I2S2INChNum / 2;
		for(uint8_t i = 0; i < InSrcCnt; i++)
		{
			if(!(I2S2_IN_LineMask & (1 << i)))
			{
				continue;
			}
			AudioCoreSourceEnable(InSourceNumMap[i]);
			if(!I2SDpllPolicy_ShouldBypassSra(I2S2_MODULE, (CFG_I2S2_MIX_MODE == I2S_SLAVE_MODE)))
			{
				AudioCoreSourceAdjust(InSourceNumMap[i], TRUE);
			}
		}
	}
	#else
	AudioCoreSourceEnable(I2S2_MIX_SOURCE_NUM);
	if(!I2SDpllPolicy_ShouldBypassSra(I2S2_MODULE, (CFG_I2S2_MIX_MODE == I2S_SLAVE_MODE)))
	{
		AudioCoreSourceAdjust(I2S2_MIX_SOURCE_NUM, TRUE);
	}
	#endif
#endif
#ifdef CFG_I2S2_MIX_OUT_EN
	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	{
		const uint8_t OutSinkNumMap[] = {
			I2S2MIXPLURALCH0_SINK_NUM, I2S2MIXPLURALCH1_SINK_NUM,
			I2S2MIXPLURALCH2_SINK_NUM, I2S2MIXPLURALCH3_SINK_NUM,
			I2S2MIXPLURALCH4_SINK_NUM, I2S2MIXPLURALCH5_SINK_NUM,
			I2S2MIXPLURALCH6_SINK_NUM, I2S2MIXPLURALCH7_SINK_NUM
		};
		uint8_t OutSinkCnt = I2S2OUTChNum / 2;
		for(uint8_t i = 0; i < OutSinkCnt; i++)
		{
			if(!(I2S2_OUT_LineMask & (1 << i)))
			{
				continue;
			}
			AudioCoreSinkEnable(OutSinkNumMap[i]);
			if(!I2SDpllPolicy_ShouldBypassSra(I2S2_MODULE, (CFG_I2S2_MIX_MODE == I2S_SLAVE_MODE)))
			{
				AudioCoreSinkAdjust(OutSinkNumMap[i], TRUE);
			}
		}
	}
	#else
	AudioCoreSinkEnable(AUDIO_I2S2_MIX_OUT_SINK_NUM);
	if(!I2SDpllPolicy_ShouldBypassSra(I2S2_MODULE, (CFG_I2S2_MIX_MODE == I2S_SLAVE_MODE)))
	{
		AudioCoreSinkAdjust(AUDIO_I2S2_MIX_OUT_SINK_NUM, TRUE);
	}
	#endif
#endif
	sI2S2MixStreamsEnabled = TRUE;
}

static void I2S2MixStreamsDisable(void)
{
	if(!sI2S2MixStreamsEnabled)
	{
		return;
	}
#ifdef CFG_I2S2_MIX_OUT_EN
	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	{
		const uint8_t OutSinkNumMap[] = {
			I2S2MIXPLURALCH0_SINK_NUM, I2S2MIXPLURALCH1_SINK_NUM,
			I2S2MIXPLURALCH2_SINK_NUM, I2S2MIXPLURALCH3_SINK_NUM,
			I2S2MIXPLURALCH4_SINK_NUM, I2S2MIXPLURALCH5_SINK_NUM,
			I2S2MIXPLURALCH6_SINK_NUM, I2S2MIXPLURALCH7_SINK_NUM
		};
		uint8_t OutSinkCnt = I2S2OUTChNum / 2;
		for(uint8_t i = 0; i < OutSinkCnt; i++)
		{
			if(!(I2S2_OUT_LineMask & (1 << i)))
			{
				continue;
			}
			AudioCoreSinkDisable(OutSinkNumMap[i]);
		}
	}
	#else
	AudioCoreSinkDisable(AUDIO_I2S2_MIX_OUT_SINK_NUM);
	#endif
#endif
#ifdef CFG_I2S2_MIX_IN_EN
	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	{
		const uint8_t InSourceNumMap[] = {
			I2S2MIXPLURALCH0_SOURCE_NUM, I2S2MIXPLURALCH1_SOURCE_NUM,
			I2S2MIXPLURALCH2_SOURCE_NUM, I2S2MIXPLURALCH3_SOURCE_NUM,
			I2S2MIXPLURALCH4_SOURCE_NUM, I2S2MIXPLURALCH5_SOURCE_NUM,
			I2S2MIXPLURALCH6_SOURCE_NUM, I2S2MIXPLURALCH7_SOURCE_NUM
		};
		uint8_t InSrcCnt = I2S2INChNum / 2;
		for(uint8_t i = 0; i < InSrcCnt; i++)
		{
			if(!(I2S2_IN_LineMask & (1 << i)))
			{
				continue;
			}
			AudioCoreSourceDisable(InSourceNumMap[i]);
		}
	}
	#else
	AudioCoreSourceDisable(I2S2_MIX_SOURCE_NUM);
	#endif
#endif
	sI2S2MixStreamsEnabled = FALSE;
}
#endif

bool I2S2_MixInit(bool hf_mode_flag)
{
#if defined(CFG_I2S2_MIX_OUT_EN) && ((I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN))
	I2S2OUTChNum = I2S2_MIX_OUT_CHNUM;
#endif
#if defined(CFG_I2S2_MIX_IN_EN) && ((I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN))
	I2S2INChNum = I2S2_MIX_IN_CHNUM;
#endif

	AudioCoreIO	AudioIOSet;
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
	sI2S2MixStreamsEnabled = FALSE;
#endif

	// ====== FIFO 申请段：集中申请 IN/OUT 所需的 SwitchBuf 和 FIFO ======
#ifdef CFG_I2S2_MIX_IN_EN
	uint8_t I2S2MixIn_Net;
	uint32_t sampleRate_MixIn;
	uint32_t FrameSize_MixIn;
#endif
#ifdef CFG_I2S2_MIX_OUT_EN
	uint8_t I2S2MixOut_Net;
	uint32_t sampleRate_MixOut;
	uint32_t MixOut_Depth;
#endif

	// --- IN FIFO ---
#ifdef CFG_I2S2_MIX_IN_EN
	if (hf_mode_flag == FALSE)
	{
	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	I2S2MixIn_Net = AudioCoreGetNetBySourceChannelNUM(I2S2MIXPLURALCH0_SOURCE_NUM);
	#else
	I2S2MixIn_Net = AudioCoreGetNetBySourceChannelNUM(I2S2_MIX_SOURCE_NUM);
	#endif
	sampleRate_MixIn  = AudioCoreMixSampleRateGet(I2S2MixIn_Net);
	FrameSize_MixIn   = AudioCoreFrameSizeGet(I2S2MixIn_Net);

	if(!I2s2MixInMalloc(FrameSize_MixIn))
	{
		APP_DBG("malloc I2S2 IN SwitchBuf error\n");
		return FALSE;
	}

	mainAppCt.I2S2_MIX_RX_FIFO_LEN = ((uint64_t)CFG_I2S2_MIX_SAMPLERATE
		* AudioCoreFrameSizeGet(I2S2MixIn_Net) * 2 / sampleRate_MixIn)
		* sizeof(PCM_DATA_TYPE) * 2 * 2 * (I2S2_MIX_IN_CHNUM / 2);
	mainAppCt.I2S2_MIX_RX_FIFO = (uint32_t*)osPortMalloc(mainAppCt.I2S2_MIX_RX_FIFO_LEN);//I2S mix rx fifo
	if (mainAppCt.I2S2_MIX_RX_FIFO != NULL)
	{
		memset(mainAppCt.I2S2_MIX_RX_FIFO, 0, mainAppCt.I2S2_MIX_RX_FIFO_LEN);
	}
	else
	{
		APP_DBG("malloc I2S2 RX FIFO error\n");
		return FALSE;
	}
	}
#endif

	// --- OUT FIFO ---
#ifdef CFG_I2S2_MIX_OUT_EN
	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	I2S2MixOut_Net = AudioCoreGetNetBySinkChannelNUM(I2S2MIXPLURALCH0_SINK_NUM);
	#else
	I2S2MixOut_Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_I2S2_MIX_OUT_SINK_NUM);
	#endif
	if(!I2s2MixOutMalloc(AudioCoreFrameSizeGet(I2S2MixOut_Net)))
	{
		APP_DBG("malloc I2S2 OUT SwitchBuf error\n");
		return FALSE;
	}
	sampleRate_MixOut = AudioCoreMixSampleRateGet(I2S2MixOut_Net);
	MixOut_Depth = (uint64_t)CFG_I2S2_MIX_SAMPLERATE
		* AudioCoreFrameSizeGet(I2S2MixOut_Net) * 2 / sampleRate_MixOut;

#if (BT_LE_AUDIO_SUPPORT && !defined(ROBOEFFECT_DOUBLE_ENGINE_EN))	//双图下面不关闭硬件操作，buffer缓存到BRIDGE_MUSIC_SINK
	mainAppCt.I2S2_MIX_TX_FIFO_LEN = get_bis_sync_need_samples(MixOut_Depth* 2 *(I2S2_MIX_OUT_CHNUM /2)) * sizeof(PCM_DATA_TYPE) * 2;
#else
	mainAppCt.I2S2_MIX_TX_FIFO_LEN = MixOut_Depth * sizeof(PCM_DATA_TYPE) * 2* 2 *(I2S2_MIX_OUT_CHNUM /2);
#endif		
	mainAppCt.I2S2_MIX_TX_FIFO = (uint32_t*)osPortMalloc(mainAppCt.I2S2_MIX_TX_FIFO_LEN);//I2S fifo
	if(mainAppCt.I2S2_MIX_TX_FIFO != NULL)
	{
		memset(mainAppCt.I2S2_MIX_TX_FIFO, 0, mainAppCt.I2S2_MIX_TX_FIFO_LEN);
	}
	else
	{
		APP_DBG("malloc I2S2 TX FIFO error\n");
		return FALSE;
	}
#endif

	I2SParamCt i2s_set;
	memset(&i2s_set, 0, sizeof(I2SParamCt));
	I2S_ModuleDisable(I2S2_MODULE);
	I2s2MixHardParamSet(&i2s_set);

#ifdef CFG_I2S2_MIX_OUT_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = MixOut_Depth;
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	#if ((CFG_I2S2_MIX_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S2_MIX_SYNC_EN))
	{//master 或者关微调
		if(CFG_I2S2_MIX_SAMPLERATE == sampleRate_MixOut)
			AudioIOSet.Adapt = STD;
		else
			AudioIOSet.Adapt = SRC_ONLY;
	}
	#else
	{//slave
	#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
		if(I2SDpllPolicy_ShouldBypassSra(I2S2_MODULE, TRUE))
		{
			AudioIOSet.Adapt = STD;
		}
		else
		{
			if(CFG_I2S2_MIX_SAMPLERATE == sampleRate_MixOut)
				AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;
			else
				AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
		}
	#else
		if(CFG_I2S2_MIX_SAMPLERATE == sampleRate_MixOut)
			AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;
		else
			AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
	#endif
	}
	#endif

	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	#ifndef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		AudioIOSet.Adapt = STD;
	#endif
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(I2S2MIXPLURALCH0_SINK_NUM);
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_I2S2_MIX_SAMPLERATE;//实选
		AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

		const uint8_t OutSinkNumMap[] = {
			I2S2MIXPLURALCH0_SINK_NUM, I2S2MIXPLURALCH1_SINK_NUM,
			I2S2MIXPLURALCH2_SINK_NUM, I2S2MIXPLURALCH3_SINK_NUM,
			I2S2MIXPLURALCH4_SINK_NUM, I2S2MIXPLURALCH5_SINK_NUM,
			I2S2MIXPLURALCH6_SINK_NUM, I2S2MIXPLURALCH7_SINK_NUM
		};
		uint16_t (* const OutPutDataFuncs[])(void*, uint16_t) = {
			AudioI2S2_PluralCH0_PutData, AudioI2S2_PluralCH1_PutData,
			AudioI2S2_PluralCH2_PutData, AudioI2S2_PluralCH3_PutData,
			AudioI2S2_PluralCH4_PutData, AudioI2S2_PluralCH5_PutData,
			AudioI2S2_PluralCH6_PutData, AudioI2S2_PluralCH7_PutData
		};
		uint16_t (* const OutGetSpaceFuncs[])(void) = {
			AudioI2S2_PluralCH0_GetSpaceLen, AudioI2S2_PluralCH1_GetSpaceLen,
			AudioI2S2_PluralCH2_GetSpaceLen, AudioI2S2_PluralCH3_GetSpaceLen,
			AudioI2S2_PluralCH4_GetSpaceLen, AudioI2S2_PluralCH5_GetSpaceLen,
			AudioI2S2_PluralCH6_GetSpaceLen, AudioI2S2_PluralCH7_GetSpaceLen
		};
		uint8_t OutSinkCnt = I2S2OUTChNum / 2;
		for (uint8_t i = 0; i < OutSinkCnt; i++)
		{
			if (!(I2S2_OUT_LineMask & (1 << i)))  // 该线被屏蔽，跳过Sink初始化
				continue;
			if (!AudioCoreSinkIsInit(OutSinkNumMap[i]))
			{
				AudioIOSet.Sync = TRUE;
				AudioIOSet.SrcQuality = SRC_QUALITY;
				AudioIOSet.DataIOFunc = OutPutDataFuncs[i];
				AudioIOSet.LenGetFunc = OutGetSpaceFuncs[i];
				if (!AudioCoreSinkInit(&AudioIOSet, OutSinkNumMap[i]))
				{
					DBG("I2S2 PLURALCH%d play source error!\n", i);
					return FALSE;
				}
			}
		#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
			AudioCoreSinkEnable(OutSinkNumMap[i]);
			AudioCoreSinkAdjust(OutSinkNumMap[i], TRUE);
		#endif
		}

	#else
		if(!AudioCoreSinkIsInit(AUDIO_I2S2_MIX_OUT_SINK_NUM))
		{

		#if((CFG_I2S2_MIX_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S2_MIX_SYNC_EN))
			{// Master 或不开微调
				if(CFG_I2S2_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S2MixOut_Net))
					AudioIOSet.Adapt = STD;//SRC_ONLY
				else
					AudioIOSet.Adapt = SRC_ONLY;
			}
		#else//slave
			{
			#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
				if(I2SDpllPolicy_ShouldBypassSra(I2S2_MODULE, TRUE))
				{
					AudioIOSet.Adapt = STD;
				}
				else
				{
					if(CFG_I2S2_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S2MixOut_Net))
						AudioIOSet.Adapt = SRA_ONLY;//SRA_ONLY;//CLK_ADJUST_ONLY;
					else
						AudioIOSet.Adapt = SRC_SRA;//SRC_SRA;//SRC_ADJUST;
				}
			#else
				if(CFG_I2S2_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S2MixOut_Net))
					AudioIOSet.Adapt = SRA_ONLY;//SRA_ONLY;//CLK_ADJUST_ONLY;
				else
					AudioIOSet.Adapt = SRC_SRA;//SRC_SRA;//SRC_ADJUST;
			#endif
			}
		#endif

			AudioIOSet.SrcQuality = SRC_QUALITY;
			AudioIOSet.Sync = TRUE;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
			AudioIOSet.Channels = 2;
			AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_I2S2_MIX_OUT_SINK_NUM);
			AudioIOSet.HighLevelCent = 60;
			AudioIOSet.LowLevelCent = 40;
			AudioIOSet.SampleRate = CFG_I2S2_MIX_SAMPLERATE;//实选
			AudioIOSet.DataIOFunc = AudioI2S2_DataSet;
			AudioIOSet.LenGetFunc = AudioI2S2_DataSpaceLenGet;
			AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

			if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_I2S2_MIX_OUT_SINK_NUM))
			{
				DBG("I2S2 Mix out init error\n");
				return FALSE;
			}
		#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
			AudioCoreSinkEnable(AUDIO_I2S2_MIX_OUT_SINK_NUM);
			AudioCoreSinkAdjust(AUDIO_I2S2_MIX_OUT_SINK_NUM, TRUE);
		#endif
		}
		else//sam add,20230221
		{
			I2S_SampleRateSet(I2S2_MODULE, AudioCoreMixSampleRateGet(I2S2MixOut_Net));
			AudioCore.AudioSink[AUDIO_I2S2_MIX_OUT_SINK_NUM].PcmWidth	= AudioIOSet.PcmWidth;
		#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
			AudioCore.AudioSink[AUDIO_I2S2_MIX_OUT_SINK_NUM].Sync = TRUE;
		#endif
		}
	#endif
#endif


#ifdef CFG_I2S2_MIX_IN_EN
	if (hf_mode_flag == FALSE)  // 蓝牙通话下不做i2s mix in
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

	#if ((CFG_I2S2_MIX_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S2_MIX_SYNC_EN))
		{//master 或者关微调
			if(CFG_I2S2_MIX_SAMPLERATE == sampleRate_MixIn)
				AudioIOSet.Adapt = STD;
			else
				AudioIOSet.Adapt = SRC_ONLY;
		}
	#else
		{//slave
		#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
			if(I2SDpllPolicy_ShouldBypassSra(I2S2_MODULE, TRUE))
			{
				AudioIOSet.Adapt = STD;
			}
			else
			{
				if(CFG_I2S2_MIX_SAMPLERATE == sampleRate_MixIn)
					AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;//
				else
					AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
			}
		#else
			if(CFG_I2S2_MIX_SAMPLERATE == sampleRate_MixIn)
				AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;//
			else
				AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
		#endif
		}
	#endif

	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	#ifndef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		AudioIOSet.Adapt = STD;
	#endif
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = I2S2MixIn_Net;
		AudioIOSet.Depth = AudioCoreFrameSizeGet(I2S2MixIn_Net) * 2;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_I2S2_MIX_SAMPLERATE;//实选
		AudioIOSet.PcmWidth = CFG_DEFAULT_WIDTH;

		const uint8_t InSourceNumMap[] = {
			I2S2MIXPLURALCH0_SOURCE_NUM, I2S2MIXPLURALCH1_SOURCE_NUM,
			I2S2MIXPLURALCH2_SOURCE_NUM, I2S2MIXPLURALCH3_SOURCE_NUM,
			I2S2MIXPLURALCH4_SOURCE_NUM, I2S2MIXPLURALCH5_SOURCE_NUM,
			I2S2MIXPLURALCH6_SOURCE_NUM, I2S2MIXPLURALCH7_SOURCE_NUM
		};
		uint16_t (* const InDataGetFuncs[])(void*, uint16_t) = {
			AudioI2S2_PluralCH0_DataGet, AudioI2S2_PluralCH1_DataGet,
			AudioI2S2_PluralCH2_DataGet, AudioI2S2_PluralCH3_DataGet,
			AudioI2S2_PluralCH4_DataGet, AudioI2S2_PluralCH5_DataGet,
			AudioI2S2_PluralCH6_DataGet, AudioI2S2_PluralCH7_DataGet
		};
		uint16_t (* const InDataLenFuncs[])(void) = {
			AudioI2S2_PluralCH0_DataLenGet, AudioI2S2_PluralCH1_DataLenGet,
			AudioI2S2_PluralCH2_DataLenGet, AudioI2S2_PluralCH3_DataLenGet,
			AudioI2S2_PluralCH4_DataLenGet, AudioI2S2_PluralCH5_DataLenGet,
			AudioI2S2_PluralCH6_DataLenGet, AudioI2S2_PluralCH7_DataLenGet
		};
		uint8_t InSrcCnt = I2S2INChNum / 2;
		for (uint8_t i = 0; i < InSrcCnt; i++)
		{
			if (!(I2S2_IN_LineMask & (1 << i)))  // 该线被屏蔽，跳过Source初始化
			continue;
			if (!AudioCoreSourceIsInit(InSourceNumMap[i]))
			{
				AudioIOSet.Sync = TRUE;
				AudioIOSet.SrcQuality = SRC_QUALITY;
				AudioIOSet.DataIOFunc = InDataGetFuncs[i];
				AudioIOSet.LenGetFunc = InDataLenFuncs[i];
				if (!AudioCoreSourceInit(&AudioIOSet, InSourceNumMap[i]))
				{
					DBG("I2S2 PLURALCH%d play source error!\n", i);
					return FALSE;
				}
			}
		#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
			AudioCoreSourceEnable(InSourceNumMap[i]);
			AudioCoreSourceAdjust(InSourceNumMap[i], TRUE);
		#endif
		}

	#else
		if(!AudioCoreSourceIsInit(I2S2_MIX_SOURCE_NUM))
		{

			AudioIOSet.SrcQuality = SRC_QUALITY;
			AudioIOSet.Sync = FALSE;//
			AudioIOSet.Channels = 2;
			AudioIOSet.Net = I2S2MixIn_Net;
			AudioIOSet.Depth = AudioCoreFrameSizeGet(I2S2MixIn_Net) * 2;
			AudioIOSet.HighLevelCent = 60;
			AudioIOSet.LowLevelCent = 40;
			AudioIOSet.SampleRate = CFG_I2S2_MIX_SAMPLERATE;//实选
			AudioIOSet.DataIOFunc = AudioI2S2_DataGet ;
			AudioIOSet.LenGetFunc = AudioI2S2_DataLenGet;
			AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
			if(!AudioCoreSourceInit(&AudioIOSet, I2S2_MIX_SOURCE_NUM))
			{
				DBG("I2S2_MIX play source error!\n");
				return FALSE;
			}
		}
	#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
		AudioCoreSourceEnable(I2S2_MIX_SOURCE_NUM);
		AudioCoreSourceAdjust(I2S2_MIX_SOURCE_NUM, TRUE);
	#endif
	#endif
	}
#endif
#if defined(I2S2_MIX_IN_THRESHOLD_TRIGER_EN)
	void I2S2MixInThresholdInterrupt(void);
    DMA_CircularThresholdLenSet(PERIPHERAL_1_ID_I2S2_RX, ((uint64_t)CFG_I2S2_MIX_SAMPLERATE * AudioCoreFrameSizeGet(I2S2MixIn_Net) / AudioCoreMixSampleRateGet(I2S2MixIn_Net)) * sizeof(PCM_DATA_TYPE) * 2);
	DMA_InterruptFunSet(PERIPHERAL_1_ID_I2S2_RX, DMA_THRESHOLD_INT, I2S2MixInThresholdInterrupt);
	#if defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
	if(I2S2MixIn_Net == CFG_PARA_CORE2_NET)
	{
		AudioCore.NetCt[I2S2MixIn_Net].TrigerMsgFunc = SendEffectServiceMsg;
	}
	else
	#endif
	{
		AudioCore.NetCt[I2S2MixIn_Net].TrigerMsgFunc = SendAudioCoreServiceMsg;
	}
	CoreInterruptEnable(INT_DMA1_T);
	DMA_InterruptEnable(PERIPHERAL_1_ID_I2S2_RX, DMA_THRESHOLD_INT, 1);
	NVIC_EnableIRQ(TMR4_DMAC1TINT_IRQn);
#endif

#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
	I2SDpllPolicy_RegisterCandidate(I2S_DPLL_OWNER_I2S2_MIX, I2S2_MODULE,
			(CFG_I2S2_MIX_MODE == I2S_SLAVE_MODE), TRUE);
	I2SDpllPolicy_RegisterStreamCallback(I2S_DPLL_OWNER_I2S2_MIX,
			I2S2MixStreamsEnable, I2S2MixStreamsDisable);
	I2SDpllPolicy_RequestOwner(I2S_DPLL_OWNER_I2S2_MIX, I2S2_MODULE,
			CFG_I2S2_MIX_SAMPLERATE, (CFG_I2S2_MIX_MODE == I2S_SLAVE_MODE));
	if(!I2SDpllPolicy_ShouldDelayStreamEnable(I2S2_MODULE, (CFG_I2S2_MIX_MODE == I2S_SLAVE_MODE)))
	{
		I2S2MixStreamsEnable();
	}
#endif

	return TRUE;
}

void I2S2_MixDeinit(void)
{
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
	I2SDpllPolicy_ReleaseOwner(I2S_DPLL_OWNER_I2S2_MIX, I2S2_MODULE);
	I2SDpllPolicy_RegisterStreamCallback(I2S_DPLL_OWNER_I2S2_MIX, NULL, NULL);
#endif
#ifdef CFG_I2S2_MIX_IN_EN
	AudioI2S_RXDeInit(I2S2_MODULE);
	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
		const uint8_t InSourceNumMap[] = {
			I2S2MIXPLURALCH0_SOURCE_NUM, I2S2MIXPLURALCH1_SOURCE_NUM,
			I2S2MIXPLURALCH2_SOURCE_NUM, I2S2MIXPLURALCH3_SOURCE_NUM,
			I2S2MIXPLURALCH4_SOURCE_NUM, I2S2MIXPLURALCH5_SOURCE_NUM,
			I2S2MIXPLURALCH6_SOURCE_NUM, I2S2MIXPLURALCH7_SOURCE_NUM
		};
		uint8_t InSrcCnt = I2S2INChNum / 2;
		for (uint8_t i = 0; i < InSrcCnt; i++)
		{
			if (!(I2S2_IN_LineMask & (1 << i)))  // 该线被屏蔽，跳过
				continue;
			AudioCoreSourceDisable(InSourceNumMap[i]);
			AudioCoreSourceDeinit(InSourceNumMap[i]);
		}
	#else
		if(AudioCoreSourceIsInit(I2S2_MIX_SOURCE_NUM))
		{
			AudioCoreSourceDisable(I2S2_MIX_SOURCE_NUM);
			AudioCoreSourceDeinit(I2S2_MIX_SOURCE_NUM);
		}
	#endif

	#if (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	if(I2S2_IN_Switch_Buf != NULL)
	{
		osPortFree(I2S2_IN_Switch_Buf);
		I2S2_IN_Switch_Buf = NULL;
	}
	#ifdef CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
	I2S2_IN_Switch_FrameSize = 0;
	for(uint8_t i = 0; i < 8; i++)
	{
		if(I2S2_IN_LineFifo[i] != NULL)
		{
			osPortFree(I2S2_IN_LineFifo[i]);
			I2S2_IN_LineFifo[i] = NULL;
		}
		memset(&I2S2_IN_LineCircularBuf[i], 0, sizeof(I2S2_IN_LineCircularBuf[i]));
	}
	#endif
	#endif

	if(mainAppCt.I2S2_MIX_RX_FIFO != NULL)
	{
		osPortFree(mainAppCt.I2S2_MIX_RX_FIFO);
		mainAppCt.I2S2_MIX_RX_FIFO = NULL;
	}
#endif

#ifdef CFG_I2S2_MIX_OUT_EN
	AudioI2S_TXDeInit(I2S2_MODULE);
	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
		const uint8_t OutSinkNumMap[] = {
			I2S2MIXPLURALCH0_SINK_NUM, I2S2MIXPLURALCH1_SINK_NUM,
			I2S2MIXPLURALCH2_SINK_NUM, I2S2MIXPLURALCH3_SINK_NUM,
			I2S2MIXPLURALCH4_SINK_NUM, I2S2MIXPLURALCH5_SINK_NUM,
			I2S2MIXPLURALCH6_SINK_NUM, I2S2MIXPLURALCH7_SINK_NUM
		};
		uint8_t OutSinkCnt = I2S2OUTChNum / 2;
		for (uint8_t i = 0; i < OutSinkCnt; i++)
		{
			if (!(I2S2_OUT_LineMask & (1 << i)))  // 该线被屏蔽，跳过
				continue;
			AudioCoreSinkDisable(OutSinkNumMap[i]);
			AudioCoreSinkDeinit(OutSinkNumMap[i]);
		}
	#else
		if(AudioCoreSinkIsInit(AUDIO_I2S2_MIX_OUT_SINK_NUM))
		{
			AudioCoreSinkDisable(AUDIO_I2S2_MIX_OUT_SINK_NUM);
			AudioCoreSinkDeinit(AUDIO_I2S2_MIX_OUT_SINK_NUM);
		}
	#endif

	#if (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)
	if(I2S2_OUT_Switch_Buf != NULL)
	{
		osPortFree(I2S2_OUT_Switch_Buf);
		I2S2_OUT_Switch_Buf = NULL;
	}
	#ifdef CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
   	I2S2_OUT_Switch_FrameSize = 0;
	for(uint8_t i = 0; i < 8; i++)
	{
		if(I2S2_OUT_LineFifo[i] != NULL)
		{
			osPortFree(I2S2_OUT_LineFifo[i]);
			I2S2_OUT_LineFifo[i] = NULL;
		}
		memset(&I2S2_OUT_LineCircularBuf[i], 0, sizeof(I2S2_OUT_LineCircularBuf[i]));
	}
	#endif
	#endif

	if(mainAppCt.I2S2_MIX_TX_FIFO != NULL)
	{
		APP_DBG("I2S2 MIX FIFO\n");
		osPortFree(mainAppCt.I2S2_MIX_TX_FIFO);
		mainAppCt.I2S2_MIX_TX_FIFO = NULL;
	}

#endif
}
#endif // CFG_FUNC_I2S2_MIX
