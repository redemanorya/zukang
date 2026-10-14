#include <string.h>
#include "type.h"
#include "dma.h"
#include "clk.h"
#include "gpio.h"
#include "i2s_interface.h"
#include "reset.h"
#include "timeout.h"
#include "debug.h"
#ifdef CFG_APP_CONFIG
	#include "audio_core_api.h"
#else
	#define PCM_DATA_TYPE	int32_t
#endif
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

static const float BclkDivTable[] = {
	1.0f,   /* 0:  i2s_clk       */
	1.5f,   /* 1:  i2s_clk/1.5   */
	2.0f,   /* 2:  i2s_clk/2     */
	3.0f,   /* 3:  i2s_clk/3     */
	4.0f,   /* 4:  i2s_clk/4     */
	5.0f,   /* 5:  i2s_clk/5     */
	5.5f,   /* 6:  i2s_clk/5.5   */
	6.0f,   /* 7:  i2s_clk/6     */
	8.0f,   /* 8:  i2s_clk/8     */
	10.0f,  /* 9:  i2s_clk/10    */
	11.0f,  /* 10: i2s_clk/11    */
	12.0f,  /* 11: i2s_clk/12    */
	16.0f,  /* 12: i2s_clk/16    */
	20.0f,  /* 13: i2s_clk/20    */
	22.0f,  /* 14: i2s_clk/22    */
	24.0f,  /* 15: i2s_clk/24    */
	25.0f,  /* 16: i2s_clk/25    */
	30.0f,  /* 17: i2s_clk/30    */
	32.0f,  /* 18: i2s_clk/32    */
	44.0f,  /* 19: i2s_clk/44    */
	48.0f,  /* 20: i2s_clk/48    */
};

static uint8_t I2s0_MasterMode = 0;
static uint8_t I2s1_MasterMode = 0;
static uint8_t I2s2_MasterMode = 0;
static I2S_DATA_LENGTH I2s0_WordLength = 0;
static I2S_DATA_LENGTH I2s1_WordLength = 0;
static I2S_DATA_LENGTH I2s2_WordLength = 0;
static bool AudioI2SDpllFollowSaved[AUDIO_DSD + 1] = {0};
static MCLK_CLK_SEL AudioI2SDpllFollowPrevSel[AUDIO_DSD + 1] = {0};
static uint32_t AudioI2SDpllCurrentSampleRate = 0U;

static PLL_CLK_INDEX AudioI2SDpllOutputClkGet(void)
{
	return PLL_CLK_5;
}

static MCLK_CLK_SEL AudioI2SDpllOutputMclkSelGet(void)
{
	return PLL_CLOCK5;
}

static MCLK_CLK_SEL AudioI2SDpllSampleRateFamilyGet(uint32_t SampleRate)
{
	return IsSelectMclkClk1(SampleRate) ? PLL_CLOCK1 : PLL_CLOCK2;
}

static DPLL_REF_CLK_SRC_SEL AudioI2SDpllRefSrcGet(I2S_MODULE Module)
{
	if(Module == I2S0_MODULE)
	{
		return DPLL_REF_CLK_SRC_GPIO_I2S0_BCLK;
	}
	else if(Module == I2S1_MODULE)
	{
		return DPLL_REF_CLK_SRC_GPIO_I2S1_BCLK;
	}

	return DPLL_REF_CLK_SRC_GPIO_I2S2_BCLK;
}

static void AudioI2SDpllFollowAudioModuleEnable(AUDIO_MODULE Module)
{
	if(!AudioI2SDpllFollowSaved[Module])
	{
		AudioI2SDpllFollowPrevSel[Module] = Clock_AudioMclkGet(Module);
		AudioI2SDpllFollowSaved[Module] = TRUE;
	}
	Clock_AudioMclkSel(Module, AudioI2SDpllOutputMclkSelGet());
}

static void AudioI2SDpllFollowAudioModuleDisable(AUDIO_MODULE Module)
{
	if(AudioI2SDpllFollowSaved[Module])
	{
		Clock_AudioMclkSel(Module, AudioI2SDpllFollowPrevSel[Module]);
		AudioI2SDpllFollowSaved[Module] = FALSE;
	}
}

bool AudioI2SDpllFollowModuleMatchSampleRate(AUDIO_MODULE Module, uint32_t SampleRate)
{
	MCLK_CLK_SEL ExpectedSel = AudioI2SDpllSampleRateFamilyGet(SampleRate);
	MCLK_CLK_SEL CurrentSel = Clock_AudioMclkGet(Module);

	if(AudioI2SDpllFollowSaved[Module])
	{
		CurrentSel = AudioI2SDpllFollowPrevSel[Module];
	}

	return CurrentSel == ExpectedSel;
}

void AudioI2S_Init(I2S_MODULE Module, I2SParamCt *ct)
{
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_1, AUDIO_PLL_CLK1_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_2, AUDIO_PLL_CLK2_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_3, AUDIO_PLL_CLK3_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_4, AUDIO_PLL_CLK4_FREQ);

	if(Module == I2S0_MODULE)
	{
		Clock_Module2Enable(I2S0_REG_CLK_EN);
		Clock_Module2Enable(I2S0_CLK_EN);
		
	}
	else if(Module == I2S1_MODULE)
	{
		Clock_Module2Enable(I2S1_REG_CLK_EN);
		Clock_Module2Enable(I2S1_CLK_EN);
	}
	else
	{
		Clock_Module2Enable(I2S2_REG_CLK_EN);
		Clock_Module2Enable(I2S2_CLK_EN);
	}

	AUDIO_MODULE AUDIOMODULE;

	I2S_MclkDivSet(Module, 2);
	I2S_MclkFreqSet(Module, AUDIO_PLL_CLK1_FREQ, AUDIO_PLL_CLK2_FREQ);

	if(Module == I2S0_MODULE)
	{
		AUDIOMODULE = AUDIO_I2S0;
	}
	else if(Module == I2S1_MODULE)
	{
		AUDIOMODULE = AUDIO_I2S1;
	}
	else
	{
		AUDIOMODULE = AUDIO_I2S2;
	}

	if(IsSelectMclkClk1(ct->SampleRate))
	{
		Clock_AudioMclkSel(AUDIOMODULE, PLL_CLOCK1);
	}
	else
	{
		Clock_AudioMclkSel(AUDIOMODULE, PLL_CLOCK2);
	}

	//AudioI2S_MclkFreqSet(Module, 2);

	//tx
	if(ct->I2sTxRxEnable & 0x1)
	{
		DMA_ChannelDisable(ct->TxPeripheralID);
		DMA_CircularConfig(ct->TxPeripheralID, ct->TxLen/2, ct->TxBuf, ct->TxLen);
	}

	if(ct->I2sTxRxEnable & 0x2)
	{
		DMA_ChannelDisable(ct->RxPeripheralID);
		DMA_CircularConfig(ct->RxPeripheralID, ct->RxLen/2, ct->RxBuf, ct->RxLen);
	}

	if(Module == I2S0_MODULE)
    {
		I2S_FadeTimeSet(I2S0_MODULE,90);
		I2S_FadeEnable(I2S0_MODULE);
		I2s0_MasterMode = ct->IsMasterMode;
		I2s0_WordLength = ct->I2sBits;
    }
    else if(Module == I2S1_MODULE)
    {
		I2S_FadeTimeSet(I2S1_MODULE,90);
		I2S_FadeEnable(I2S1_MODULE);
		I2s1_MasterMode = ct->IsMasterMode;
		I2s1_WordLength = ct->I2sBits;
    }
    else
    {
		I2S_FadeTimeSet(I2S2_MODULE,90);
		I2S_FadeEnable(I2S2_MODULE);
		I2s2_MasterMode = ct->IsMasterMode;
		I2s2_WordLength = ct->I2sBits;
    }

	if(ct->LineMode == Single_Mode)
	{
		I2S_DualModeSet(Module, 0);
		I2S_QuadModeSet(Module, 0);
	}
	else if(ct->LineMode == Dual_Mode)
	{
		I2S_DualModeSet(Module, 1);
		I2S_QuadModeSet(Module, 0);
	}
	else
	{
		I2S_DualModeSet(Module, 0);
		if(ct->I2sFormat != I2S_FORMAT_TDM)
		{
			I2S_QuadModeSet(Module, 1);
		}
		else
		{
			I2S_QuadModeSet(Module, 0);
			DBG("TDM does not support Quad Mode\n");
		}
	}

	if(ct->I2sFormat == I2S_FORMAT_TDM)
	{
		TDM_CHANNEL_SET(Module, ct->TdmChNum);
	}

	I2S_AlignModeSet(Module, ct->ALIGN_MODE);
	I2S_SampleRateSet(Module, ct->SampleRate);

	if(ct->IsMasterMode == 0)
	{
		I2S_MasterModeSet(Module, ct->I2sFormat, ct->I2sBits);
	}
	else
	{
		I2S_SlaveModeSet(Module, ct->I2sFormat, ct->I2sBits);
	}

	//TDM 时钟重新配置
	if(ct->I2sFormat == I2S_FORMAT_TDM && ct->IsMasterMode == 0)
	{
		uint32_t ch_per_line = (uint32_t)ct->TdmChNum;
		if(ct->LineMode == Dual_Mode)
		{
			ch_per_line = ch_per_line / 2;
		}

		uint32_t required_bclk = ch_per_line * ct->SampleRate * 32;
		uint32_t i2s_clk = I2S_MclkFreqGet(Module);

		int32_t best_idx = -1;
		for(int32_t i = 0; i < 21; i++)
		{
			if((float)i2s_clk / BclkDivTable[i] >= (float)required_bclk)
				best_idx = i;
			else
				break;
		}

		if(best_idx < 0)
		{
			DBG("TDM BCLK insufficient! need=%lu Hz, i2s_clk=%lu Hz, ch/line=%lu, fs=%lu\n",
				required_bclk, i2s_clk, ch_per_line, ct->SampleRate);
			best_idx = 0;
		}

		uint32_t actual_bclk = (uint32_t)((float)i2s_clk / BclkDivTable[best_idx]);
		uint32_t lrclk_div   = actual_bclk / ct->SampleRate;

		I2S_BclkFreqSet(Module, (uint32_t)best_idx);
		I2S_LrclkFreqSet(Module, lrclk_div);
	}
	
	if(ct->I2sTxRxEnable & 0x1)
	{
		DMA_ChannelEnable(ct->TxPeripheralID);
		I2S_ModuleTxEnable(Module);
	}
	if(ct->I2sTxRxEnable & 0x2)
	{
		DMA_ChannelEnable(ct->RxPeripheralID);
		I2S_ModuleRxEnable(Module);
	}
	I2S_ModuleEnable(Module);
}

void AudioI2S_RXDeInit(I2S_MODULE Module)
{
	TIMER Request_cnt;
	uint8_t PeripheralID;

	if(Module == I2S0_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_I2S0_RX;
	}
	else if(Module == I2S1_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_I2S1_RX;
	}
	else
	{
		PeripheralID = PERIPHERAL_1_ID_I2S2_RX;
	}

	TimeOutSet(&Request_cnt, 3);
	while(I2S_RXBUSY_StatusGet(Module) != 0)
	{
		if(IsTimeOut(&Request_cnt))
		{
			break;
		}
	}

	I2S_ModuleRxDisable(Module);
	DMA_InterruptFlagClear(PeripheralID, DMA_DONE_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_ERROR_INT);
	DMA_CircularFIFOClear(PeripheralID);
	DMA_ChannelDisable(PeripheralID);
}

void AudioI2S_TXDeInit(I2S_MODULE Module)
{
	TIMER Request_cnt;
	uint8_t PeripheralID;

	if(Module == I2S0_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_I2S0_TX;
	}
	else if(Module == I2S1_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_I2S1_TX;
	}
	else
	{
		PeripheralID = PERIPHERAL_1_ID_I2S2_TX;
	}
	I2S_ModuleTxPause(Module, 1);
	TimeOutSet(&Request_cnt, 3);
	while(I2S_TXBUSY_StatusGet(Module) != 0)
	{
		if(IsTimeOut(&Request_cnt))
		{
			break;
		}
	}

	I2S_ModuleTxDisable(Module);
	DMA_InterruptFlagClear(PeripheralID, DMA_DONE_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_ERROR_INT);
	DMA_CircularFIFOClear(PeripheralID);
	DMA_ChannelDisable(PeripheralID);
	I2S_ModuleTxPause(Module, 0);
}

void AudioI2S_DeInit(I2S_MODULE Module)
{
	if(Module == I2S0_MODULE)
    {
		I2S_ModuleDisable(I2S0_MODULE);
		Reset_Register1Reset(I2S0_REG_SEPA);
		Reset_Function1Reset(I2S0_FUNC_SEPA);

		DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S0_RX, DMA_DONE_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S0_RX, DMA_THRESHOLD_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S0_RX, DMA_ERROR_INT);
    	DMA_CircularFIFOClear(PERIPHERAL_1_ID_I2S0_RX);
    	DMA_ChannelDisable(PERIPHERAL_1_ID_I2S0_RX);
		
		DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S0_TX, DMA_DONE_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S0_TX, DMA_THRESHOLD_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S0_TX, DMA_ERROR_INT);
    	DMA_CircularFIFOClear(PERIPHERAL_1_ID_I2S0_TX);
        DMA_ChannelDisable(PERIPHERAL_1_ID_I2S0_TX);
    }
    else if(Module == I2S1_MODULE)
    {
    	I2S_ModuleDisable(I2S1_MODULE);
    	Reset_Register1Reset(I2S1_REG_SEPA);
		Reset_Function1Reset(I2S1_FUNC_SEPA);

		DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S1_RX, DMA_DONE_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S1_RX, DMA_THRESHOLD_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S1_RX, DMA_ERROR_INT);
    	DMA_CircularFIFOClear(PERIPHERAL_1_ID_I2S1_RX);
    	DMA_ChannelDisable(PERIPHERAL_1_ID_I2S1_RX);
		
		DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S1_TX, DMA_DONE_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S1_TX, DMA_THRESHOLD_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S1_TX, DMA_ERROR_INT);
    	DMA_CircularFIFOClear(PERIPHERAL_1_ID_I2S1_TX);
        DMA_ChannelDisable(PERIPHERAL_1_ID_I2S1_TX);
		
    }
    else
    {
    	I2S_ModuleDisable(I2S2_MODULE);
    	Reset_Register1Reset(I2S2_REG_SEPA);
		Reset_Function2Reset(I2S2_FUNC_SEPA);

		DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S2_RX, DMA_DONE_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S2_RX, DMA_THRESHOLD_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S2_RX, DMA_ERROR_INT);
    	DMA_CircularFIFOClear(PERIPHERAL_1_ID_I2S2_RX);
    	DMA_ChannelDisable(PERIPHERAL_1_ID_I2S2_RX);

		DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S2_TX, DMA_DONE_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S2_TX, DMA_THRESHOLD_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S2_TX, DMA_ERROR_INT);
    	DMA_CircularFIFOClear(PERIPHERAL_1_ID_I2S2_TX);
        DMA_ChannelDisable(PERIPHERAL_1_ID_I2S2_TX);
    }
}

uint8_t AudioI2S_MasterModeGet(I2S_MODULE Module)
{
	if(Module == I2S0_MODULE)
    {
		return I2s0_MasterMode;
    }
    else if(Module == I2S1_MODULE)
    {
		return I2s1_MasterMode;
    }
    else
    {
    	return I2s2_MasterMode;
    }
}

I2S_DATA_LENGTH AudioI2S_WordLengthGet(I2S_MODULE Module)
{
	if(Module == I2S0_MODULE)
    {
		return I2s0_WordLength;
    }
    else if(Module == I2S1_MODULE)
    {
		return I2s1_WordLength;
    }
    else
    {
    	return I2s2_WordLength;
    }
}

//返回Length uint：sample
//16bits:  NumSamples / 4;
//>16bits: NumSamples / 8;
uint16_t AudioI2S_DataLenGet(I2S_MODULE Module)
{
	uint16_t NumSamples = 0;
	uint16_t databits = AudioI2S_WordLengthGet(Module);

	if(Module == I2S0_MODULE)
    {
		NumSamples = DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S0_RX);
    }
    else if(Module == I2S1_MODULE)
    {
    	NumSamples = DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S1_RX);
    }
    else
    {
    	NumSamples = DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX);
    }

	return databits == I2S_LENGTH_16BITS? NumSamples / 4 : NumSamples / 8;
}

//返回Length uint：sample
uint16_t AudioI2S_DataGet(I2S_MODULE Module, void* Buf, uint16_t Len)
{
	uint16_t Length = 0;
	uint16_t databits = AudioI2S_WordLengthGet(Module);

    if(Module == I2S0_MODULE)
    {
    	Length = DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S0_RX);
    }
    else if(Module == I2S1_MODULE)
    {
    	Length = DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S1_RX);
    }
    else
    {
    	Length = DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_RX);
    }

    if(databits > I2S_LENGTH_16BITS)
    	Len <<= 1;

	if(Length > Len * 4)
	{
		Length = Len * 4;
	}

    if(Module == I2S0_MODULE)
	{
		DMA_CircularDataGet(PERIPHERAL_1_ID_I2S0_RX, Buf, Length & (databits > I2S_LENGTH_16BITS ? 0xFFFFFFF8 : 0xFFFFFFFC));
	}
	else if(Module == I2S1_MODULE)
	{
		DMA_CircularDataGet(PERIPHERAL_1_ID_I2S1_RX, Buf, Length & (databits > I2S_LENGTH_16BITS ? 0xFFFFFFF8 : 0xFFFFFFFC));
	}
	else
	{
		DMA_CircularDataGet(PERIPHERAL_1_ID_I2S2_RX, Buf, Length & (databits > I2S_LENGTH_16BITS ? 0xFFFFFFF8 : 0xFFFFFFFC));
	}

    return databits == I2S_LENGTH_16BITS? Length / 4 : Length / 8;
}


uint16_t AudioI2S_DataSpaceLenGet(I2S_MODULE Module)
{
	uint16_t databits = AudioI2S_WordLengthGet(Module);

	if(Module == I2S0_MODULE)
	{
		return databits == I2S_LENGTH_16BITS? DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S0_TX) / 4 : DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S0_TX) / 8;
	}
	else if(Module == I2S1_MODULE)
	{
		return databits == I2S_LENGTH_16BITS? DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S1_TX) / 4 : DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S1_TX) / 8;
	}
	else
	{
		return databits == I2S_LENGTH_16BITS? DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX) / 4 : DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_I2S2_TX) / 8;
	}
}

void AudioI2S_DataSet(I2S_MODULE Module, void *Buf, uint32_t Len)
{
	uint16_t Length = 0;
	uint16_t databits = AudioI2S_WordLengthGet(Module);

	Length = databits == I2S_LENGTH_16BITS? Len * 4 : Len * 8;//Byte

	if(Module == I2S0_MODULE)
	{
		DMA_CircularDataPut(PERIPHERAL_1_ID_I2S0_TX, Buf, Length & 0xFFFFFFFC);
	}
	else if(Module == I2S1_MODULE)
	{
		DMA_CircularDataPut(PERIPHERAL_1_ID_I2S1_TX, Buf, Length & 0xFFFFFFFC);
	}
	else
	{
		DMA_CircularDataPut(PERIPHERAL_1_ID_I2S2_TX, Buf, Length & 0xFFFFFFFC);
	}
}


//for 应用层
uint16_t AudioI2S0_DataLenGet()
{
	return AudioI2S_DataLenGet(I2S0_MODULE);
}

uint16_t AudioI2S1_DataLenGet()
{
	return AudioI2S_DataLenGet(I2S1_MODULE);
}

uint16_t AudioI2S2_DataLenGet()
{
	return AudioI2S_DataLenGet(I2S2_MODULE);
}

uint16_t AudioI2S0_DataGet(void* Buf, uint16_t Len)
{
	return AudioI2S_DataGet(I2S0_MODULE, Buf, Len);
}

uint16_t AudioI2S1_DataGet(void* Buf, uint16_t Len)
{
	return AudioI2S_DataGet(I2S1_MODULE, Buf, Len);
}

uint16_t AudioI2S2_DataGet(void* Buf, uint16_t Len)
{
	return AudioI2S_DataGet(I2S2_MODULE, Buf, Len);
}

uint16_t AudioI2S0_DataSpaceLenGet()
{
	return AudioI2S_DataSpaceLenGet(I2S0_MODULE);
}

uint16_t AudioI2S1_DataSpaceLenGet()
{
	return AudioI2S_DataSpaceLenGet(I2S1_MODULE);
}

uint16_t AudioI2S2_DataSpaceLenGet()
{
	return AudioI2S_DataSpaceLenGet(I2S2_MODULE);
}

uint16_t AudioI2S0_DataSet(void *Buf, uint16_t Len)
{
	AudioI2S_DataSet(I2S0_MODULE, Buf, Len);
	return 0;    //为应用层接口统一
}

uint16_t AudioI2S1_DataSet(void *Buf, uint16_t Len)
{
	AudioI2S_DataSet(I2S1_MODULE, Buf, Len);
	return 0;    //为应用层接口统一
}

uint16_t AudioI2S2_DataSet(void *Buf, uint16_t Len)
{
	AudioI2S_DataSet(I2S2_MODULE, Buf, Len);
	return 0;    //为应用层接口统一
}

uint16_t AudioI2S0_TX_DataLenGet()
{
	uint8_t SampleByte = (AudioI2S_WordLengthGet(I2S0_MODULE) == I2S_LENGTH_16BITS)? 4:8;
	return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S0_TX) / SampleByte;
}

uint16_t AudioI2S1_TX_DataLenGet()
{
	uint8_t SampleByte = (AudioI2S_WordLengthGet(I2S1_MODULE) == I2S_LENGTH_16BITS) ? 4:8;
	return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S1_TX) / SampleByte;
}

uint16_t AudioI2S2_TX_DataLenGet()
{
	uint8_t SampleByte = (AudioI2S_WordLengthGet(I2S2_MODULE) == I2S_LENGTH_16BITS) ? 4:8;
	return DMA_CircularDataLenGet(PERIPHERAL_1_ID_I2S2_TX) / SampleByte;
}

void AudioI2S_SampleRateChange(I2S_MODULE Module, uint32_t SampleRate)
{
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_1, AUDIO_PLL_CLK1_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_2, AUDIO_PLL_CLK2_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_3, AUDIO_PLL_CLK3_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_4, AUDIO_PLL_CLK4_FREQ);

	I2S_MclkDivSet(Module, 2);
	I2S_MclkFreqSet(Module, AUDIO_PLL_CLK1_FREQ, AUDIO_PLL_CLK2_FREQ);

	if(Module == I2S0_MODULE)
    {
		if(IsSelectMclkClk1(SampleRate))
		{
			Clock_AudioMclkSel(AUDIO_I2S0, PLL_CLOCK1);
		}
		else
		{
			Clock_AudioMclkSel(AUDIO_I2S0, PLL_CLOCK2);
		}
		I2S_SampleRateSet(Module, SampleRate);
    }
    else if(Module == I2S1_MODULE)
    {
    	if(IsSelectMclkClk1(SampleRate))
    	{
			Clock_AudioMclkSel(AUDIO_I2S1, PLL_CLOCK1);
    	}
		else
		{
			Clock_AudioMclkSel(AUDIO_I2S1, PLL_CLOCK2);
		}
    	I2S_SampleRateSet(Module, SampleRate);
    }
	else if(Module == I2S2_MODULE)
	{
		if(IsSelectMclkClk1(SampleRate))
    	{
			Clock_AudioMclkSel(AUDIO_I2S2, PLL_CLOCK1);
    	}
		else
		{
			Clock_AudioMclkSel(AUDIO_I2S2, PLL_CLOCK2);
		}
    	I2S_SampleRateSet(Module, SampleRate);
	}
}

uint32_t AudioI2SDpllLockFreqGet(uint32_t SampleRate)
{
	if((SampleRate == 11025U) || (SampleRate == 22050U) || (SampleRate == 44100U)
			|| (SampleRate == 88200U) || (SampleRate == 176400U))
	{
		return 225792000U;
	}

	return 245760000U;//245760000U;//368640000U
}

uint32_t AudioI2SDpllOutputFreqGet(uint32_t SampleRate)
{
	if((SampleRate == 11025U) || (SampleRate == 22050U) || (SampleRate == 44100U)
			|| (SampleRate == 88200U) || (SampleRate == 176400U))
	{
		return AUDIO_PLL_CLK1_FREQ;
	}

	return AUDIO_PLL_CLK2_FREQ;
}

void AudioI2SDpllTargetFreqConfig(uint32_t SampleRate)
{
	AudioI2SDpllCurrentSampleRate = SampleRate;
	Clock_DPllLock(DPLL0_MODULE, AudioI2SDpllLockFreqGet(SampleRate));
	// SREG_DPLL0_CTRL.RST_N = 0;
}

void AudioI2SDpllRefInit(I2S_MODULE Module)
{
	(void)Module;
	Clock_Module1Enable(DPLL0_CLK_EN);
	Clock_PLLModule1Enable(SRC2_PLL_CLK_EN | SRC2_APLL_CLK_EN | SRC2_DPLL0_CLK_EN);
	Clock_PLLModule2Enable(SRC2_REFO_PLL_CLK_EN);
	Clock_DPll0Release_ExtRef();
}

void AudioI2SDpllRefDeInit(void)
{
	Clock_DPll0HoldSet_ExtRef(FALSE);
	Clock_DPll0Release_ExtRef();
	AudioI2SDpllCurrentSampleRate = 0U;
}

bool AudioI2SDpllRefStart(I2S_MODULE Module, uint32_t SampleRate)
{
	AudioI2SDpllCurrentSampleRate = SampleRate;
	return Clock_Dpll0LockStart_ExtRef(AudioI2SDpllRefSrcGet(Module), SampleRate, AudioI2SDpllLockFreqGet(SampleRate), FALSE);
}

bool AudioI2SDpllRefLockStatusGet(void)
{
	return Clock_DPll0LockStatusGet_ExtRef();
}

void AudioI2SDpllRefHold(void)
{
	Clock_DPll0HoldSet_ExtRef(TRUE);
	Clock_AudioPllClockSet(DPLL0_CLK_MODE, AudioI2SDpllOutputClkGet(),
			AudioI2SDpllOutputFreqGet(AudioI2SDpllCurrentSampleRate));
}

void AudioI2SDpllRefHoldSet(bool IsEnable)
{
	Clock_DPll0HoldSet_ExtRef(IsEnable);
}

bool AudioI2SDpllRefHoldGet(void)
{
	return Clock_DPll0HoldGet_ExtRef();
}

void AudioI2SDpllFollowEnable(uint32_t FollowMask)
{
	if(FollowMask == AUDIO_I2S_DPLL_FOLLOW_NONE)
	{
		return;
	}
	APP_DBG("AudioI2SDpllFollowEnable[0x%lx]\n",FollowMask);
	// Clock_AudioPllClockSet(DPLL0_CLK_MODE, AudioI2SDpllOutputClkGet(),
	// 		AudioI2SDpllOutputFreqGet(AudioI2SDpllCurrentSampleRate));//当前不需要、保留api，用于后续DPLL0定频后延时跟随功能

	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_DAC0)
	{
		AudioI2SDpllFollowAudioModuleEnable(AUDIO_DAC0);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_DAC1)
	{
		AudioI2SDpllFollowAudioModuleEnable(AUDIO_DAC1);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_DAC2)
	{
		AudioI2SDpllFollowAudioModuleEnable(AUDIO_DAC2);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_ADC0)
	{
		AudioI2SDpllFollowAudioModuleEnable(AUDIO_ADC0);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_ADC1)
	{
		AudioI2SDpllFollowAudioModuleEnable(AUDIO_ADC1);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_I2S0)
	{
		AudioI2SDpllFollowAudioModuleEnable(AUDIO_I2S0);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_I2S1)
	{
		AudioI2SDpllFollowAudioModuleEnable(AUDIO_I2S1);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_I2S2)
	{
		AudioI2SDpllFollowAudioModuleEnable(AUDIO_I2S2);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_SPDIF0)
	{
		Clock_Spdif0ClkSelect(DPLL0_CLK_MODE);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_SPDIF1)
	{
		Clock_Spdif1ClkSelect(DPLL0_CLK_MODE);
	}
}

void AudioI2SDpllFollowDisable(uint32_t FollowMask)
{
	APP_DBG("AudioI2SDpllFollowDisable[0x%lx]\n",FollowMask);
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_DAC0)
	{
		AudioI2SDpllFollowAudioModuleDisable(AUDIO_DAC0);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_DAC1)
	{
		AudioI2SDpllFollowAudioModuleDisable(AUDIO_DAC1);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_DAC2)
	{
		AudioI2SDpllFollowAudioModuleDisable(AUDIO_DAC2);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_ADC0)
	{
		AudioI2SDpllFollowAudioModuleDisable(AUDIO_ADC0);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_ADC1)
	{
		AudioI2SDpllFollowAudioModuleDisable(AUDIO_ADC1);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_I2S0)
	{
		AudioI2SDpllFollowAudioModuleDisable(AUDIO_I2S0);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_I2S1)
	{
		AudioI2SDpllFollowAudioModuleDisable(AUDIO_I2S1);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_I2S2)
	{
		AudioI2SDpllFollowAudioModuleDisable(AUDIO_I2S2);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_SPDIF0)
	{
		Clock_Spdif0ClkSelect(APLL_CLK_MODE);
	}
	if(FollowMask & AUDIO_I2S_DPLL_FOLLOW_SPDIF1)
	{
		Clock_Spdif1ClkSelect(APLL_CLK_MODE);
	}
}

void AudioI2SDpllFollowDisableAll(void)
{
	AudioI2SDpllFollowDisable(AUDIO_I2S_DPLL_FOLLOW_DAC0 | AUDIO_I2S_DPLL_FOLLOW_DAC1
			| AUDIO_I2S_DPLL_FOLLOW_DAC2 | AUDIO_I2S_DPLL_FOLLOW_ADC0
			| AUDIO_I2S_DPLL_FOLLOW_ADC1 | AUDIO_I2S_DPLL_FOLLOW_I2S0
			| AUDIO_I2S_DPLL_FOLLOW_I2S1 | AUDIO_I2S_DPLL_FOLLOW_I2S2
			| AUDIO_I2S_DPLL_FOLLOW_SPDIF0 | AUDIO_I2S_DPLL_FOLLOW_SPDIF1);
}

//GPIO配置见 "gpio_config.h"，芯片实际端口要一致
#define I2S0_MCLK_GPIO					MCLK_OUT_A4 //MCLK_OUT_A6//MCLK_OUT_A14
#define I2S1_MCLK_GPIO					MCLK_OUT_A20 //MCLK_OUT_B4//MCLK_OUT_B10
/*
 *GPIO端口输出MCLK主频，MCLK0和I2S0相关，MCLK1和I2S1相关
 *
 *I2S2没有MCLK out复用端口
 *MCLK0和MCLK1均可以输出48K和44.1KHz的整数倍
 *默认Mclk0为44.1K倍数，mclk1为48K倍数
 *GPIO输入MCLK频率和具体I2S模块无关
 *
 *此API I2S对应模块使用，但是想要输出MCLK时钟时调用
 */
void AudioI2S_MclkOutInit(I2S_MODULE Module,uint32_t SampleRate)
{
	AUDIO_MODULE AUDIOMODULE;

	if(Module == I2S0_MODULE)
	{
		Clock_Module2Enable(I2S0_REG_CLK_EN);
		Clock_Module2Enable(I2S0_CLK_EN);
	}
	else if(Module == I2S1_MODULE)
	{
		Clock_Module2Enable(I2S1_REG_CLK_EN);
		Clock_Module2Enable(I2S1_CLK_EN);
	}

	I2S_MclkDivSet(Module, 2);//B6 PLL0和PLL1默认配置为12.288M/11.2896MHz的2倍，所以这里要2分频
	I2S_MclkFreqSet(Module, AUDIO_PLL_CLK1_FREQ, AUDIO_PLL_CLK2_FREQ);

	if(Module == I2S0_MODULE)
	{
		AUDIOMODULE = AUDIO_I2S0;
		if(!IsSelectMclkClk1(SampleRate))
		{
			I2S_MclkFreqSet(Module, AUDIO_PLL_CLK2_FREQ, AUDIO_PLL_CLK2_FREQ);//mclk0设置为12.288Mhz倍数
		}

		SET_I2S_PIN_MODE((I2S0_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_MCLK_GPIO));//mclk
	}
	else if(Module == I2S1_MODULE)
	{
		AUDIOMODULE = AUDIO_I2S1;
		if(IsSelectMclkClk1(SampleRate))
		{
			I2S_MclkFreqSet(Module, AUDIO_PLL_CLK1_FREQ, AUDIO_PLL_CLK1_FREQ);//mclk1设置为11.2896Mhz倍数
		}

		SET_I2S_PIN_MODE((I2S1_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S1_MCLK_GPIO));//mclk
	}

	if(IsSelectMclkClk1(SampleRate))
	{
		Clock_AudioMclkSel(AUDIOMODULE, PLL_CLOCK1);
	}
	else
	{
		Clock_AudioMclkSel(AUDIOMODULE, PLL_CLOCK2);
	}
}

#ifdef CFG_APP_CONFIG
	#if (defined(I2S1_MIX_IN_LINE_CNT) && (I2S1_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE)) \
		|| (defined(I2S1_MIX_OUT_LINE_CNT) && (I2S1_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE)) \
		|| (defined(I2S2_MIX_IN_LINE_CNT) && (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE)) \
		|| (defined(I2S2_MIX_OUT_LINE_CNT) && (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE)) || defined(CFG_FUNC_I2S1_TDM_MODE_EN) || defined(CFG_FUNC_I2S2_TDM_MODE_EN)

uint16_t AudioI2SDataRXCarry(I2S_MODULE Module, uint8_t ChNum, uint8_t LineMask, int32_t* I2S_Switch_Buf, uint16_t Len)
{
	int32_t DataLen;

	uint8_t dma_channel = 0;
	switch(Module)
	{
		case I2S0_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S0_RX;
			break;
		case I2S1_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S1_RX;
			break;
		case I2S2_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S2_RX;
			break;
		default:
			DBG("Invalid I2S module\n");
			return 0;
	}

	// 获取可用DMA数据长度
	DataLen = DMA_CircularDataLenGet(dma_channel);
	//DBG("Data=%d\n", DataLen);
	if(DataLen < Len * sizeof(PCM_DATA_TYPE) * ChNum)
	{
		//DBG("tdm_len = 0\n");
		return 0;
	}

	// 从DMA读取数据（始终按全部ChNum读取）
	DMA_CircularDataGet(dma_channel, I2S_Switch_Buf, Len * sizeof(PCM_DATA_TYPE) * ChNum);

	// ChNum个通道，每2个通道共用一个I2SPLURALCHx_SOURCE_NUM
	// 分离声道，直接写入各Source的PcmInBuf
	// LineMask: bit[a]=0 表示该线被屏蔽，对应DMA数据直接丢弃
	const uint8_t SourceNumMap[] = {I2SPLURALCH0_SOURCE_NUM, I2SPLURALCH1_SOURCE_NUM, I2SPLURALCH2_SOURCE_NUM, I2SPLURALCH3_SOURCE_NUM, \
			I2SPLURALCH4_SOURCE_NUM, I2SPLURALCH5_SOURCE_NUM, I2SPLURALCH6_SOURCE_NUM, I2SPLURALCH7_SOURCE_NUM};
	PCM_DATA_TYPE *pInput = (PCM_DATA_TYPE *)I2S_Switch_Buf;
	for(uint16_t i = 0; i < Len; i++)
	{
		for(int a = 0; a < ChNum/2; a++)
		{
			if ((LineMask & (1 << a)) && AudioCoreSourceIsEnable(SourceNumMap[a]))
			{
				AudioCoreSource *Src = &AudioCore.AudioSource[SourceNumMap[a]];
				Src->PcmInBuf[2*i]     = pInput[ChNum * i + a*2];     // L
				Src->PcmInBuf[2*i + 1] = pInput[ChNum * i + a*2 + 1]; // R
			}
			// 被屏蔽或未使能的线：DMA数据直接丢弃
		}
	}

	return Len;
}

void AudioI2SDataTXCarry(I2S_MODULE Module, uint8_t ChNum, uint8_t LineMask, int32_t* I2S_Switch_Buf, uint16_t Len)
{
	int32_t SpaceLen;

	uint8_t dma_channel = 0;
	switch(Module)
	{
		case I2S0_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S0_TX;
			break;
		case I2S1_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S1_TX;
			break;
		case I2S2_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S2_TX;
			break;
		default:
			DBG("Invalid I2S module\n");
			return;
	}

	// 获取可用DMA数据长度
	SpaceLen = DMA_CircularSpaceLenGet(dma_channel);
	//DBG("Space=%d\n", SpaceLen);
	if(SpaceLen < Len * sizeof(PCM_DATA_TYPE) * ChNum)
	{
		//DBG("tdm_len = 0\n");
		return;
	}

		// ChNum个通道，每2个通道共用一个I2SPLURALCHx_SINK_NUM
		// 把各Sink的PcmOutBuf数据按顺序组装到I2S_Switch_Buf
		// LineMask: bit[a]=0 表示该线被屏蔽，对应位置填0
		const uint8_t SinkNumMap[] = {I2SPLURALCH0_SINK_NUM, I2SPLURALCH1_SINK_NUM, I2SPLURALCH2_SINK_NUM, I2SPLURALCH3_SINK_NUM, \
				I2SPLURALCH4_SINK_NUM, I2SPLURALCH5_SINK_NUM, I2SPLURALCH6_SINK_NUM, I2SPLURALCH7_SINK_NUM};
		PCM_DATA_TYPE *pOutput = (PCM_DATA_TYPE *)I2S_Switch_Buf;
		for(uint16_t i = 0; i < Len; i++)
		{
			for(int a = 0; a < ChNum/2; a++)
			{
				if ((LineMask & (1 << a)) && AudioCoreSinkIsEnable(SinkNumMap[a]))
				{
					AudioCoreSink *Sink = &AudioCore.AudioSink[SinkNumMap[a]];
					pOutput[ChNum * i + a*2]     = Sink->PcmOutBuf[2*i];     // L
					pOutput[ChNum * i + a*2 + 1] = Sink->PcmOutBuf[2*i + 1]; // R
				}
				else
				{
					// 被屏蔽或未使能的线填0（DMA仍按全部ChNum搬运）
					pOutput[ChNum * i + a*2]     = 0;
					pOutput[ChNum * i + a*2 + 1] = 0;
				}
			}
		}

	DMA_CircularDataPut(dma_channel, I2S_Switch_Buf, Len * sizeof(PCM_DATA_TYPE) * ChNum);
	//DBG(".");
}

	#endif
#else

uint16_t AudioI2SDataRXCarry(I2S_MODULE Module, uint8_t ChNum, uint8_t LineMask, int32_t* I2S_Switch_Buf, uint16_t Len)
{
	int32_t DataLen;

	uint8_t dma_channel = 0;
	switch(Module)
	{
		case I2S0_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S0_RX;
			break;
		case I2S1_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S1_RX;
			break;
		case I2S2_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S2_RX;
			break;
		default:
			DBG("Invalid I2S module\n");
			return 0;
	}

	(void)LineMask;   //消除警告
	// 获取可用DMA数据长度
	DataLen = DMA_CircularDataLenGet(dma_channel);
	if(DataLen < Len * sizeof(PCM_DATA_TYPE) * ChNum)
	{
		return 0;
	}

	// 从DMA读取原始TDM交织数据到用户缓冲区
	DMA_CircularDataGet(dma_channel, I2S_Switch_Buf, Len * sizeof(PCM_DATA_TYPE) * ChNum);

	return Len;
}

void AudioI2SDataTXCarry(I2S_MODULE Module, uint8_t ChNum, uint8_t LineMask, int32_t* I2S_Switch_Buf, uint16_t Len)
{
	int32_t SpaceLen;

	uint8_t dma_channel = 0;
	switch(Module)
	{
		case I2S0_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S0_TX;
			break;
		case I2S1_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S1_TX;
			break;
		case I2S2_MODULE:
			dma_channel = PERIPHERAL_1_ID_I2S2_TX;
			break;
		default:
			DBG("Invalid I2S module\n");
			return;
	}

	(void)LineMask;  //消除警告
	// 获取DMA可用空间
	SpaceLen = DMA_CircularSpaceLenGet(dma_channel);
	if(SpaceLen < Len * sizeof(PCM_DATA_TYPE) * ChNum)
	{
		return;
	}

	// 直接写入DMA（交织由调用者完成）
	DMA_CircularDataPut(dma_channel, I2S_Switch_Buf, Len * sizeof(PCM_DATA_TYPE) * ChNum);
}
#endif
