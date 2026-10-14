#include <string.h>
#include "debug.h"
#include "i2s_dpll_policy.h"

#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN

#define I2S_DPLL_LOCK_TIMEOUT_TICKS 100U
#define I2S_DPLL_HOLD_RELEASE_TICKS 100U

typedef struct _I2SDpllCandidate
{
	bool Active;
	bool Requested;
	bool IsSlave;
	I2S_MODULE Module;
	uint32_t SampleRate;
} I2SDpllCandidate;

typedef struct _I2SDpllPolicyContext
{
	bool Enable;
	bool LockLost;
	I2S_DPLL_OWNER CurrentOwner;
	I2S_MODULE RefModule;
	uint32_t SampleRate;
	I2S_DPLL_STATE State;
	uint32_t FollowMask;
	uint16_t WaitCnt;
	uint16_t RetryCnt;
	I2SDpllCandidate Candidate[I2S_DPLL_OWNER_MAX];
	I2S_DPLL_STREAM_CB StreamEnableCb[I2S_DPLL_OWNER_MAX];
	I2S_DPLL_STREAM_CB StreamDisableCb[I2S_DPLL_OWNER_MAX];
	bool StreamEnabled[I2S_DPLL_OWNER_MAX];
} I2SDpllPolicyContext;

static I2SDpllPolicyContext sI2SDpllPolicyCt;

#if defined(CFG_RES_AUDIO_SPDIF0OUT_EN) || defined(CFG_RES_AUDIO_SPDIF1OUT_EN)
extern void SpdifOutDpllPolicySync(bool Enable);

static void I2SDpllPolicySpdifOutSync(bool Enable)
{
	SpdifOutDpllPolicySync(Enable);
}
#else
static void I2SDpllPolicySpdifOutSync(bool Enable)
{
	(void)Enable;
}
#endif

static void I2SDpllPolicyStreamDisableCurrent(void)
{
	if((sI2SDpllPolicyCt.CurrentOwner > I2S_DPLL_OWNER_NONE)
			&& (sI2SDpllPolicyCt.CurrentOwner < I2S_DPLL_OWNER_MAX)
			&& sI2SDpllPolicyCt.StreamEnabled[sI2SDpllPolicyCt.CurrentOwner]
			&& (sI2SDpllPolicyCt.StreamDisableCb[sI2SDpllPolicyCt.CurrentOwner] != NULL))
	{
		sI2SDpllPolicyCt.StreamDisableCb[sI2SDpllPolicyCt.CurrentOwner]();
		sI2SDpllPolicyCt.StreamEnabled[sI2SDpllPolicyCt.CurrentOwner] = FALSE;
		APP_DBG("I2S DPLL stream disable: owner=%u module=%u\n",
				(uint32_t)sI2SDpllPolicyCt.CurrentOwner,
				(uint32_t)sI2SDpllPolicyCt.RefModule);
	}
}

static bool I2SDpllPolicyFollowModuleAllowed(AUDIO_MODULE Module, uint32_t SampleRate)
{
	if(SampleRate == 0U)
	{
		return FALSE;
	}

	if(!AudioI2SDpllFollowModuleMatchSampleRate(Module, SampleRate))
	{
		APP_DBG("I2S DPLL follow skip: audio=%u fs=%lu mclk family mismatch\n",
				(uint32_t)Module,
				(unsigned long)SampleRate);
		return FALSE;
	}

	return TRUE;
}

static bool I2SDpllPolicyIsModuleSelected(I2S_MODULE Module)
{
	return (CFG_I2S_DPLL_REF_MODULE != I2S_DPLL_REF_MODULE_NONE) && (Module == (I2S_MODULE)CFG_I2S_DPLL_REF_MODULE);
}

static bool I2SDpllPolicyCandidateValid(I2S_DPLL_OWNER Owner)
{
	I2SDpllCandidate *Candidate;

	if(Owner <= I2S_DPLL_OWNER_NONE || Owner >= I2S_DPLL_OWNER_MAX)
	{
		return FALSE;
	}

	Candidate = &sI2SDpllPolicyCt.Candidate[Owner];
	return Candidate->Active && Candidate->Requested && Candidate->IsSlave
			&& I2SDpllPolicyIsModuleSelected(Candidate->Module) && (Candidate->SampleRate != 0U);
}

static I2S_DPLL_OWNER I2SDpllPolicyOwnerPick(void)
{
	uint32_t Owner;

	for(Owner = (uint32_t)I2S_DPLL_OWNER_I2SIN_MODE; Owner < (uint32_t)I2S_DPLL_OWNER_MAX; Owner++)
	{
		if(I2SDpllPolicyCandidateValid((I2S_DPLL_OWNER)Owner))
		{
			return (I2S_DPLL_OWNER)Owner;
		}
	}

	return I2S_DPLL_OWNER_NONE;
}

static uint32_t I2SDpllPolicyFollowMaskBuild(void)
{
	uint32_t FollowMask = AUDIO_I2S_DPLL_FOLLOW_NONE;
	I2S_MODULE RefModule = sI2SDpllPolicyCt.RefModule;
	uint32_t SampleRate = sI2SDpllPolicyCt.SampleRate;

#if CFG_AUDIO_DAC0_FOLLOW_DPLL0_EN
	if(I2SDpllPolicyFollowModuleAllowed(AUDIO_DAC0, SampleRate))
	{
		FollowMask |= AUDIO_I2S_DPLL_FOLLOW_DAC0;
	}
#endif
#if CFG_AUDIO_DAC1_FOLLOW_DPLL0_EN
	if(I2SDpllPolicyFollowModuleAllowed(AUDIO_DAC1, SampleRate))
	{
		FollowMask |= AUDIO_I2S_DPLL_FOLLOW_DAC1;
	}
#endif
#if CFG_AUDIO_DAC2_FOLLOW_DPLL0_EN
	if(I2SDpllPolicyFollowModuleAllowed(AUDIO_DAC2, SampleRate))
	{
		FollowMask |= AUDIO_I2S_DPLL_FOLLOW_DAC2;
	}
#endif
#if CFG_AUDIO_ADC0_FOLLOW_DPLL0_EN
	if(I2SDpllPolicyFollowModuleAllowed(AUDIO_ADC0, SampleRate))
	{
		FollowMask |= AUDIO_I2S_DPLL_FOLLOW_ADC0;
	}
#endif
#if CFG_AUDIO_ADC1_FOLLOW_DPLL0_EN
	if(I2SDpllPolicyFollowModuleAllowed(AUDIO_ADC1, SampleRate))
	{
		FollowMask |= AUDIO_I2S_DPLL_FOLLOW_ADC1;
	}
#endif
#if CFG_AUDIO_I2S0_FOLLOW_DPLL0_EN
	if((RefModule != I2S0_MODULE) && (AudioI2S_MasterModeGet(I2S0_MODULE) == I2S_MASTER_MODE)
			&& I2SDpllPolicyFollowModuleAllowed(AUDIO_I2S0, SampleRate))
	{
		FollowMask |= AUDIO_I2S_DPLL_FOLLOW_I2S0;
	}
#endif
#if CFG_AUDIO_I2S1_FOLLOW_DPLL0_EN
	if((RefModule != I2S1_MODULE) && (AudioI2S_MasterModeGet(I2S1_MODULE) == I2S_MASTER_MODE)
			&& I2SDpllPolicyFollowModuleAllowed(AUDIO_I2S1, SampleRate))
	{
		FollowMask |= AUDIO_I2S_DPLL_FOLLOW_I2S1;
	}
#endif
#if CFG_AUDIO_I2S2_FOLLOW_DPLL0_EN
	if((RefModule != I2S2_MODULE) && (AudioI2S_MasterModeGet(I2S2_MODULE) == I2S_MASTER_MODE)
			&& I2SDpllPolicyFollowModuleAllowed(AUDIO_I2S2, SampleRate))
	{
		FollowMask |= AUDIO_I2S_DPLL_FOLLOW_I2S2;
	}
#endif
#if defined(CFG_APP_OPTICAL_MODE_EN) || defined(CFG_APP_COAXIAL_MODE_EN)
#if CFG_AUDIO_SPDIF0OUT_FOLLOW_DPLL0_EN
	FollowMask |= AUDIO_I2S_DPLL_FOLLOW_SPDIF0;
#endif
#if CFG_AUDIO_SPDIF1OUT_FOLLOW_DPLL0_EN
	FollowMask |= AUDIO_I2S_DPLL_FOLLOW_SPDIF1;
#endif
#endif

	return FollowMask;
}

static uint32_t I2SDpllPolicySampleRatePoll(void)
{
	if((sI2SDpllPolicyCt.CurrentOwner <= I2S_DPLL_OWNER_NONE)
			|| (sI2SDpllPolicyCt.CurrentOwner >= I2S_DPLL_OWNER_MAX))
	{
		return 0U;
	}

	if(!sI2SDpllPolicyCt.Candidate[sI2SDpllPolicyCt.CurrentOwner].IsSlave)
	{
		return 0U;
	}

	return I2S_SampleRateGet(sI2SDpllPolicyCt.RefModule);
}

static void I2SDpllPolicyReleaseInternal(void)
{
	APP_DBG("I2S DPLL release: owner=%u module=%u fs=%lu follow=0x%lx retry=%u\n",
			(uint32_t)sI2SDpllPolicyCt.CurrentOwner,
			(uint32_t)sI2SDpllPolicyCt.RefModule,
			(unsigned long)sI2SDpllPolicyCt.SampleRate,
			(unsigned long)sI2SDpllPolicyCt.FollowMask,
			(uint32_t)sI2SDpllPolicyCt.RetryCnt);

	I2SDpllPolicyStreamDisableCurrent();
	I2SDpllPolicySpdifOutSync(FALSE);

	if(sI2SDpllPolicyCt.FollowMask != AUDIO_I2S_DPLL_FOLLOW_NONE)
	{
		APP_DBG("I2S DPLL follow disable: follow=0x%lx\n",
				(unsigned long)sI2SDpllPolicyCt.FollowMask);
		AudioI2SDpllFollowDisable(sI2SDpllPolicyCt.FollowMask);
		sI2SDpllPolicyCt.FollowMask = AUDIO_I2S_DPLL_FOLLOW_NONE;
	}

	AudioI2SDpllRefDeInit();
	sI2SDpllPolicyCt.CurrentOwner = I2S_DPLL_OWNER_NONE;
	sI2SDpllPolicyCt.RefModule = (I2S_MODULE)CFG_I2S_DPLL_REF_MODULE;
	sI2SDpllPolicyCt.SampleRate = 0U;
	sI2SDpllPolicyCt.State = I2S_DPLL_STATE_IDLE;
	sI2SDpllPolicyCt.WaitCnt = 0U;
	sI2SDpllPolicyCt.LockLost = FALSE;
}

void I2SDpllPolicy_Init(void)
{
	memset(&sI2SDpllPolicyCt, 0, sizeof(sI2SDpllPolicyCt));
	sI2SDpllPolicyCt.Enable = TRUE;
	sI2SDpllPolicyCt.RefModule = (I2S_MODULE)CFG_I2S_DPLL_REF_MODULE;
	sI2SDpllPolicyCt.State = I2S_DPLL_STATE_IDLE;
	APP_DBG("I2S DPLL init: enable=%u ref_sel=%u\n",
			(uint32_t)sI2SDpllPolicyCt.Enable,
			(uint32_t)CFG_I2S_DPLL_REF_MODULE);
}

void I2SDpllPolicy_RegisterCandidate(I2S_DPLL_OWNER Owner, I2S_MODULE Module, bool IsSlave, bool IsActive)
{
	if(Owner <= I2S_DPLL_OWNER_NONE || Owner >= I2S_DPLL_OWNER_MAX)
	{
		return;
	}

	sI2SDpllPolicyCt.Candidate[Owner].Module = Module;
	sI2SDpllPolicyCt.Candidate[Owner].IsSlave = IsSlave;
	sI2SDpllPolicyCt.Candidate[Owner].Active = IsActive;
	if(!IsActive)
	{
		sI2SDpllPolicyCt.Candidate[Owner].Requested = FALSE;
		sI2SDpllPolicyCt.Candidate[Owner].SampleRate = 0U;
	}

	APP_DBG("I2S DPLL candidate: owner=%u module=%u slave=%u active=%u\n",
			(uint32_t)Owner,
			(uint32_t)Module,
			(uint32_t)IsSlave,
			(uint32_t)IsActive);
}

void I2SDpllPolicy_RegisterStreamCallback(I2S_DPLL_OWNER Owner, I2S_DPLL_STREAM_CB EnableCb, I2S_DPLL_STREAM_CB DisableCb)
{
	if(Owner <= I2S_DPLL_OWNER_NONE || Owner >= I2S_DPLL_OWNER_MAX)
	{
		return;
	}

	sI2SDpllPolicyCt.StreamEnableCb[Owner] = EnableCb;
	sI2SDpllPolicyCt.StreamDisableCb[Owner] = DisableCb;
	if((EnableCb == NULL) && (DisableCb == NULL))
	{
		sI2SDpllPolicyCt.StreamEnabled[Owner] = FALSE;
	}
}

bool I2SDpllPolicy_RequestOwner(I2S_DPLL_OWNER Owner, I2S_MODULE Module, uint32_t SampleRate, bool IsSlave)
{
	if(!sI2SDpllPolicyCt.Enable || !I2SDpllPolicyIsModuleSelected(Module) || !IsSlave)
	{
		return FALSE;
	}

	I2SDpllPolicy_RegisterCandidate(Owner, Module, IsSlave, TRUE);
	sI2SDpllPolicyCt.Candidate[Owner].Requested = TRUE;
	sI2SDpllPolicyCt.Candidate[Owner].SampleRate = SampleRate;
	APP_DBG("I2S DPLL request: owner=%u module=%u fs=%lu slave=%u\n",
			(uint32_t)Owner,
			(uint32_t)Module,
			(unsigned long)SampleRate,
			(uint32_t)IsSlave);
	return TRUE;
}

void I2SDpllPolicy_ReportSampleRate(I2S_DPLL_OWNER Owner, I2S_MODULE Module, uint32_t SampleRate)
{
	if(Owner <= I2S_DPLL_OWNER_NONE || Owner >= I2S_DPLL_OWNER_MAX)
	{
		return;
	}

	sI2SDpllPolicyCt.Candidate[Owner].Module = Module;
	sI2SDpllPolicyCt.Candidate[Owner].SampleRate = SampleRate;
	if((sI2SDpllPolicyCt.CurrentOwner == Owner) && (sI2SDpllPolicyCt.SampleRate != SampleRate) && (SampleRate != 0U))
	{
		APP_DBG("I2S DPLL fs change: owner=%u module=%u %lu->%lu\n",
				(uint32_t)Owner,
				(uint32_t)Module,
				(unsigned long)sI2SDpllPolicyCt.SampleRate,
				(unsigned long)SampleRate);
		sI2SDpllPolicyCt.SampleRate = SampleRate;
		sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
	}
}

void I2SDpllPolicy_ReportClockLost(I2S_DPLL_OWNER Owner, I2S_MODULE Module)
{
	(void)Module;
	if(sI2SDpllPolicyCt.CurrentOwner == Owner)
	{
		APP_DBG("I2S DPLL lock lost: owner=%u module=%u fs=%lu\n",
				(uint32_t)Owner,
				(uint32_t)sI2SDpllPolicyCt.RefModule,
				(unsigned long)sI2SDpllPolicyCt.SampleRate);
		sI2SDpllPolicyCt.LockLost = TRUE;
		sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
	}
}

void I2SDpllPolicy_ReleaseOwner(I2S_DPLL_OWNER Owner, I2S_MODULE Module)
{
	if(Owner <= I2S_DPLL_OWNER_NONE || Owner >= I2S_DPLL_OWNER_MAX)
	{
		return;
	}

	sI2SDpllPolicyCt.Candidate[Owner].Requested = FALSE;
	sI2SDpllPolicyCt.Candidate[Owner].Active = FALSE;
	sI2SDpllPolicyCt.Candidate[Owner].SampleRate = 0U;
	APP_DBG("I2S DPLL release req: owner=%u module=%u\n",
			(uint32_t)Owner,
			(uint32_t)Module);
	if((sI2SDpllPolicyCt.CurrentOwner == Owner) && (sI2SDpllPolicyCt.RefModule == Module))
	{
		sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELEASE;
	}
}

bool I2SDpllPolicy_IsStable(void)
{
	return sI2SDpllPolicyCt.State == I2S_DPLL_STATE_STABLE;
}

bool I2SDpllPolicy_ShouldDelayStreamEnable(I2S_MODULE Module, bool IsSlave)
{
	return sI2SDpllPolicyCt.Enable && IsSlave && I2SDpllPolicyIsModuleSelected(Module)
			&& !I2SDpllPolicy_IsStable();
}

bool I2SDpllPolicy_ShouldBypassSra(I2S_MODULE Module, bool IsSlave)
{
	return sI2SDpllPolicyCt.Enable && IsSlave && I2SDpllPolicyIsModuleSelected(Module);
}

uint32_t I2SDpllPolicy_GetFollowMask(void)
{
	return sI2SDpllPolicyCt.FollowMask;
}

void I2SDpllPolicy_Process(void)
{
	I2S_DPLL_OWNER NextOwner;

	if(!sI2SDpllPolicyCt.Enable || (CFG_I2S_DPLL_REF_MODULE == I2S_DPLL_REF_MODULE_NONE))
	{
		return;
	}

	switch(sI2SDpllPolicyCt.State)
	{
		case I2S_DPLL_STATE_IDLE:
			NextOwner = I2SDpllPolicyOwnerPick();
			if(NextOwner != I2S_DPLL_OWNER_NONE)
			{
				sI2SDpllPolicyCt.CurrentOwner = NextOwner;
				sI2SDpllPolicyCt.RefModule = sI2SDpllPolicyCt.Candidate[NextOwner].Module;
				sI2SDpllPolicyCt.SampleRate = sI2SDpllPolicyCt.Candidate[NextOwner].SampleRate;
				sI2SDpllPolicyCt.WaitCnt = 0U;
				sI2SDpllPolicyCt.RetryCnt = 0U;
				APP_DBG("I2S DPLL owner pick: owner=%u module=%u fs=%lu\n",
						(uint32_t)NextOwner,
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate);
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_PREPARE;
			}
			break;

		case I2S_DPLL_STATE_PREPARE:
			APP_DBG("I2S DPLL prepare: owner=%u module=%u fs=%lu retry=%u\n",
					(uint32_t)sI2SDpllPolicyCt.CurrentOwner,
					(uint32_t)sI2SDpllPolicyCt.RefModule,
					(unsigned long)sI2SDpllPolicyCt.SampleRate,
					(uint32_t)sI2SDpllPolicyCt.RetryCnt);
			AudioI2SDpllTargetFreqConfig(sI2SDpllPolicyCt.SampleRate);
			AudioI2SDpllRefInit(sI2SDpllPolicyCt.RefModule);
			sI2SDpllPolicyCt.WaitCnt = 0U;
			sI2SDpllPolicyCt.State = I2S_DPLL_STATE_START;
			break;

		case I2S_DPLL_STATE_START:
			if(AudioI2SDpllRefStart(sI2SDpllPolicyCt.RefModule, sI2SDpllPolicyCt.SampleRate))
			{
				APP_DBG("I2S DPLL start ok: module=%u fs=%lu\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate);
				sI2SDpllPolicyCt.WaitCnt = 0U;
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_WAIT_LOCK;
			}
			else
			{
				sI2SDpllPolicyCt.RetryCnt++;
				APP_DBG("I2S DPLL start fail: module=%u fs=%lu retry=%u\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate,
						(uint32_t)sI2SDpllPolicyCt.RetryCnt);
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
			}
			break;

		case I2S_DPLL_STATE_WAIT_LOCK:
			if(AudioI2SDpllRefLockStatusGet())
			{
				APP_DBG("I2S DPLL locked: module=%u fs=%lu retry=%u\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate,
						(uint32_t)sI2SDpllPolicyCt.RetryCnt);
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_HOLD;
				sI2SDpllPolicyCt.WaitCnt = 0U;
			}
			else if(++sI2SDpllPolicyCt.WaitCnt > I2S_DPLL_LOCK_TIMEOUT_TICKS)
			{
				sI2SDpllPolicyCt.RetryCnt++;
				APP_DBG("I2S DPLL wait lock timeout: module=%u fs=%lu retry=%u\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate,
						(uint32_t)sI2SDpllPolicyCt.RetryCnt);
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
			}
			break;

		case I2S_DPLL_STATE_HOLD:
			AudioI2SDpllRefHold();
			sI2SDpllPolicyCt.FollowMask = I2SDpllPolicyFollowMaskBuild();
			APP_DBG("I2S DPLL hold: module=%u fs=%lu follow=0x%lx\n",
					(uint32_t)sI2SDpllPolicyCt.RefModule,
					(unsigned long)sI2SDpllPolicyCt.SampleRate,
					(unsigned long)sI2SDpllPolicyCt.FollowMask);
			if(sI2SDpllPolicyCt.FollowMask != AUDIO_I2S_DPLL_FOLLOW_NONE)
			{
				AudioI2SDpllFollowEnable(sI2SDpllPolicyCt.FollowMask);
			}
			sI2SDpllPolicyCt.WaitCnt = 0U;
			sI2SDpllPolicyCt.State = I2S_DPLL_STATE_WAIT_RELEASE;
			break;

		case I2S_DPLL_STATE_WAIT_RELEASE:
			if(!AudioI2SDpllRefLockStatusGet())
			{
				APP_DBG("I2S DPLL relock trigger: lock lost in wait release, module=%u fs=%lu\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate);
				sI2SDpllPolicyCt.LockLost = TRUE;
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
				break;
			}
			if(++sI2SDpllPolicyCt.WaitCnt > I2S_DPLL_HOLD_RELEASE_TICKS)
			{
				AudioI2SDpllRefHoldSet(FALSE);
				APP_DBG("I2S DPLL stable: module=%u fs=%lu\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate);
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_STABLE;
			}
			break;

		case I2S_DPLL_STATE_STABLE:
		{
			uint32_t CurrentSampleRate = I2SDpllPolicySampleRatePoll();

			if(!AudioI2SDpllRefLockStatusGet())
			{
				APP_DBG("I2S DPLL relock trigger: lock lost in stable, module=%u fs=%lu\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate);
				sI2SDpllPolicyCt.LockLost = TRUE;
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
				break;
			}

			if(sI2SDpllPolicyCt.LockLost || !I2SDpllPolicyCandidateValid(sI2SDpllPolicyCt.CurrentOwner))
			{
				APP_DBG("I2S DPLL relock trigger: owner invalid or lock lost, owner=%u module=%u fs=%lu\n",
						(uint32_t)sI2SDpllPolicyCt.CurrentOwner,
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate);
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
			}
			else if(CurrentSampleRate == 0U)
			{
				APP_DBG("I2S DPLL relock trigger: sample rate poll invalid, module=%u fs=%lu\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate);
				sI2SDpllPolicyCt.LockLost = TRUE;
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
			}
			else if(CurrentSampleRate != sI2SDpllPolicyCt.SampleRate)
			{
				APP_DBG("I2S DPLL relock trigger: poll fs change, module=%u %lu->%lu\n",
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate,
						(unsigned long)CurrentSampleRate);
				sI2SDpllPolicyCt.Candidate[sI2SDpllPolicyCt.CurrentOwner].SampleRate = CurrentSampleRate;
				sI2SDpllPolicyCt.SampleRate = CurrentSampleRate;
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELOCK;
			}
			else if((sI2SDpllPolicyCt.CurrentOwner > I2S_DPLL_OWNER_NONE)
					&& (sI2SDpllPolicyCt.CurrentOwner < I2S_DPLL_OWNER_MAX)
					&& !sI2SDpllPolicyCt.StreamEnabled[sI2SDpllPolicyCt.CurrentOwner]
					&& (sI2SDpllPolicyCt.StreamEnableCb[sI2SDpllPolicyCt.CurrentOwner] != NULL))
			{
				sI2SDpllPolicyCt.StreamEnableCb[sI2SDpllPolicyCt.CurrentOwner]();
				sI2SDpllPolicyCt.StreamEnabled[sI2SDpllPolicyCt.CurrentOwner] = TRUE;
				APP_DBG("I2S DPLL stream enable: owner=%u module=%u fs=%lu\n",
						(uint32_t)sI2SDpllPolicyCt.CurrentOwner,
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate);
			}
			I2SDpllPolicySpdifOutSync(TRUE);
			break;
		}

		case I2S_DPLL_STATE_RELOCK:
			I2SDpllPolicyStreamDisableCurrent();
			I2SDpllPolicySpdifOutSync(FALSE);
			sI2SDpllPolicyCt.LockLost = FALSE;
			if(!I2SDpllPolicyCandidateValid(sI2SDpllPolicyCt.CurrentOwner))
			{
				APP_DBG("I2S DPLL relock abort: owner invalid, owner=%u module=%u\n",
						(uint32_t)sI2SDpllPolicyCt.CurrentOwner,
						(uint32_t)sI2SDpllPolicyCt.RefModule);
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_RELEASE;
			}
			else
			{
				APP_DBG("I2S DPLL relock: owner=%u module=%u fs=%lu retry=%u\n",
						(uint32_t)sI2SDpllPolicyCt.CurrentOwner,
						(uint32_t)sI2SDpllPolicyCt.RefModule,
						(unsigned long)sI2SDpllPolicyCt.SampleRate,
						(uint32_t)sI2SDpllPolicyCt.RetryCnt);
				if(sI2SDpllPolicyCt.FollowMask != AUDIO_I2S_DPLL_FOLLOW_NONE)
				{
					APP_DBG("I2S DPLL follow disable: follow=0x%lx\n",
							(unsigned long)sI2SDpllPolicyCt.FollowMask);
					AudioI2SDpllFollowDisable(sI2SDpllPolicyCt.FollowMask);
					sI2SDpllPolicyCt.FollowMask = AUDIO_I2S_DPLL_FOLLOW_NONE;
				}
				AudioI2SDpllRefDeInit();
				sI2SDpllPolicyCt.State = I2S_DPLL_STATE_PREPARE;
			}
			break;

		case I2S_DPLL_STATE_RELEASE:
			I2SDpllPolicyReleaseInternal();
			break;

		default:
			sI2SDpllPolicyCt.State = I2S_DPLL_STATE_IDLE;
			break;
	}
}

#endif
