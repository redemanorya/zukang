#include "string.h"
#include "type.h"
#include "dac.h"
#include "dma.h"
#include "clk.h"
#include "irqn.h"
#include "debug.h"
#include "src.h"
#include "core_d1088.h"
#include "src_interface.h"


int16_t SRC_BlockInit(SRC_MODULE ModuleIndex, SRC_Context* srcContext, int32_t SampleRateIn, int32_t SampleRateOut)
{
	Clock_SRCClkDivSet(ModuleIndex, (2 << 16) | 0x8000);
	if((SampleRateIn == 384000) || (SampleRateOut == 384000))
	{
		SampleRateIn /= 2;
		SampleRateOut /= 2;
		Clock_SRCClkDivSet(ModuleIndex, (1 << 16) | 0x8000);
	}
	else if((SampleRateIn == 192000) && (SampleRateOut == 176400))
	{
		//ÓÅ»¯¸ßÆµ
		SampleRateIn = 48000;
		SampleRateOut = 44100;
		Clock_SRCClkDivSet(ModuleIndex, (1 << 16) | 0x8000);
	}
	else if((SampleRateIn == 192000) && (SampleRateOut == 88200))
	{
		SampleRateIn = 96000;
		SampleRateOut = 44100;
	}
	else if((SampleRateIn == 176400) && (SampleRateOut == 96000))
	{

		SampleRateIn = 88200;
		SampleRateOut = 48000;
	}
	else if((SampleRateIn == 96000) && (SampleRateOut == 88200))
	{

		SampleRateIn = 48000;
		SampleRateOut = 44100;
	}

	SRC_Init(ModuleIndex, srcContext, SampleRateIn, SampleRateOut);
	SRC_BlockDoneFlagClean(ModuleIndex);
	return 0;
}

int16_t SRC_BlockApply(SRC_MODULE ModuleIndex, SRC_Context* srcContext, int16_t *pcm_in, int16_t *pcm_out, int32_t n)
{
	uint32_t RealLen;//Samples
	uint32_t Bytes = (srcContext->Width == SRC_24BIT) ? 8 : 4;

	memcpy(srcContext->InBuf, pcm_in, n * Bytes);

	SRC_BlockStart(ModuleIndex);

	while(!SRC_IsBlockDone(ModuleIndex));
	RealLen = SRC_BlockDataLenGet(ModuleIndex);

	DataCacheRangeInvalid((uint32_t)srcContext->OutBuf, RealLen * Bytes);
	memcpy(pcm_out, srcContext->OutBuf, RealLen * Bytes);

	return RealLen;
}

void SRC_BlockApplyStart(SRC_MODULE ModuleIndex, SRC_Context* srcContext, int16_t *pcm_in, int32_t n)
{
	uint32_t Bytes = (srcContext->Width == SRC_24BIT) ? 8 : 4;
	memcpy(srcContext->InBuf, pcm_in, n * Bytes);

	SRC_BlockStart(ModuleIndex);
}

int16_t SRC_BlockApplyDone(SRC_MODULE ModuleIndex, SRC_Context* srcContext, int16_t *pcm_out)
{
	uint32_t RealLen;//Samples

	uint32_t Bytes = (srcContext->Width == SRC_24BIT) ? 8 : 4;
	while(!SRC_IsBlockDone(ModuleIndex));
	RealLen = SRC_BlockDataLenGet(ModuleIndex);

	DataCacheRangeInvalid((uint32_t)srcContext->OutBuf, RealLen * Bytes);
	memcpy(pcm_out, srcContext->OutBuf, RealLen * Bytes);

	SRC_BlockDoneFlagClean(ModuleIndex);

	return RealLen;
}

