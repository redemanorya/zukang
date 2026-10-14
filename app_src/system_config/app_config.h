/*
 * app_config.h
 *
 *  Created on: Sep 2, 2025
 *      Author: piwang
 */

#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_
#include "type.h"
#include "chip_info.h"
#include "chip_config.h"
#include "spi_flash.h"
#include "sys_em_config.h"
#include "bt_config.h"
#include "pmu.h"

/************************************************************************************************************
 功能宏：
 CFG_APP_*  : 切模式列表选配
 CFG_FUNC_* : 功能开关
 CFG_PARA_* : 参数设置
 CFG_RES_*  : 硬件资源配置
 默认开全局宏：CFG_APP_CONFIG / FUNC_OS_EN，IDE工程配置(Build Settings-Compiler-Symbols)
 Porting*   ：移植的功能宏以及代码段尚未验证和修缮，待完善
 Effect Help：\B6_Audio_SDK\documents\B6调音指导手册_V*.*.*.pdf
  重要帮助文档:\B6_Audio_SDK\documents\B6调音指导手册_V0.2.1.pdf
************************************************************************************************************/
//#define CFG_192KHZ_EN					//支持192K音效图（Mcps & Ram多，音效少）

/**LOW_DELAY_SAMPLE_TARGET刚性约束主通路,典型Case配:ROBOEFFECT_DOUBLE_ENGINE_EN(mic->DAC) **/
//#define CFG_LOW_DELAY_CTRL
//#define AUDIO_STREAM_UNINTERRUPT//mixer通路音频在切换音效时保持音频不断音
/********************************************************************************************************************
 * 					 AudioCore MixNet Param:帧长、采样率、PCM位宽
 * 开音效后, 帧长/采样率跟随音效图; WIDTH_L24BIT时,Flow通路允许16BIT,Flow通路都16BIT时NetWidth=16BIT.
 ******************************************************************************************************************/
#define CFG_MAX_FRAME					512
#define CFG_MIN_FRAME					64//选配[64 / 128]
#define CFG_DEFAULT_SAMPLERATE			48000//44100//
#define CFG_DEFAULT_WIDTH				WIDTH_L24BIT//勿改

/* LineIn acquisition backend. Keep both branches buildable for A/B tests. */
#define LINEIN_INPUT_INTERNAL_ADC       0
#define LINEIN_INPUT_GC1808_I2S2       1
#ifndef CFG_LINEIN_INPUT_SOURCE
#define CFG_LINEIN_INPUT_SOURCE         LINEIN_INPUT_GC1808_I2S2
#endif
#if (CFG_LINEIN_INPUT_SOURCE != LINEIN_INPUT_INTERNAL_ADC) && \
    (CFG_LINEIN_INPUT_SOURCE != LINEIN_INPUT_GC1808_I2S2)
#error "Invalid CFG_LINEIN_INPUT_SOURCE"
#endif

#define CFG_APP_IDLE_MODE_EN			//提示音和休眠用，此宏不要关闭
#define CFG_FUNC_OPEN_SLOW_DEVICE_TASK
/************************Mode List AppSource+AppSink***********************************/
#if(BT_SUPPORT)
	#define CFG_APP_BT_MODE_EN			// 经典蓝牙模式
#elif(BLE_SUPPORT)
	#define CFG_APP_BLE_MODE_EN			// BLE模式
#endif
#define CFG_APP_LINEIN_MODE_EN				// 线路模式
//#define CFG_APP_USB_PLAY_MODE_EN			// 全速U盘播放模式
#define CFG_APP_USB_HS_PLAY_MODE_EN			// 高速U盘播放模式
#define CFG_APP_CARD_PLAY_MODE_EN			// TF卡播放模式
//#define CFG_APP_USB_AUDIO_MODE_EN			// USB声卡模式关闭，HS HID/升级源码保留
//#define CFG_APP_USB_FS_AUDIO_MODE_EN		// USB声卡模式关闭，PA8/PA9仅作为FS HID
//#define CFG_APP_I2SIN_MODE_EN				// I2S In模式
#define CFG_APP_OPTICAL_MODE_EN				// SPDIF 光纤模式
#define CFG_APP_COAXIAL_MODE_EN				// SPDIF 同轴模式
#define CFG_APP_RADIOIN_MODE_EN				// FM模式
//#define CFG_APP_HDMIIN_MODE_EN

/*************************************************************************************************************
                 	 	 	 	 	 	常驻音频通路
       独立使能,模式共用@mode_task_api,
**************************************************************************************************************/
/**Sink**/
#define CFG_RES_DAC_MONITOR_EN//暂勿关闭

//#define CFG_RES_AUDIO_DAC1_MIX_OUT_EN		// 注意资源冲突
#define CFG_RES_AUDIO_DAC2_MIX_OUT_EN		//双图Music默认节拍
//#define CFG_I2S0_MIX_OUT_EN				// I2s0MixSink
#define CFG_I2S1_MIX_OUT_EN
//#define CFG_I2S2_MIX_OUT_EN				// I2s2MixSink
//#define CFG_RES_AUDIO_SPDIF0OUT_EN		// SPDIF0_SINK
//#define CFG_RES_AUDIO_SPDIF1OUT_EN		// SPDIF1_SINK

/**Headphone**/
/* console shell over UART1 for src switch etc. */
//#define CFG_FUNC_SHELL_EN                     /* UART shell disabled; source retained */
#define CFG_FUNC_OXS_PROTOCOL_EN                /* UART1 PA14/PA15, 115200 */
#define CFG_RL_SPEAKER_VOLUME_DEFAULT           66U
#define CFG_RL_HEADPHONE_VOLUME_DEFAULT         72U
#define CFG_FUNC_HEADPHONE_EN                        /* Headphone out (DAC0) + impedance detect 耳机输出和阻抗匹配开启*/
//#define CFG_HP_DAC0_BOOT_MUTE_TEST              /* Temporary diagnostic: keep DAC0 muted during boot */
#define CFG_HP_AD_REF_TOGGLE_BYPASS_TEST          /* Temporary diagnostic: keep PB8 AD reference low */
/* PB8=AD_EN, PB9=HP_DETC, PA30/31=AD0/AD1 (see gpio_config.h) */
/**Source**/
#define CFG_FUNC_MIC_EN
	#ifdef CFG_FUNC_MIC_EN   //注意：使用micbias必须接上电容(参考山景标准开发板)
	//	#define MICBIAS1_EN
	//	#define MICBIAS2_EN
	#endif

/** RemindSource **/
#define CFG_FUNC_REMIND_SOUND_EN

/*多路SOURCE MIX配置选项   //资源消耗多*/
#define	CFG_FUNC_MULTISOURCE_EN
#ifdef CFG_FUNC_MULTISOURCE_EN
	#ifndef CFG_APP_LINEIN_MODE_EN
//		#define CFG_FUNC_LINEIN_MIX_MODE	// LineinMixSource
	#endif
//	#define CFG_I2S0_MIX_IN_EN				// I2s0MixSouce
//	#define CFG_I2S1_MIX_IN_EN				// I2s1MixSource
//	#define CFG_I2S2_MIX_IN_EN				// I2s2MixSource
//	#define CFG_RES_USB_FS_MIX_AUDIO_EN		// for FS device audio
//	#define CFG_RES_USB_HS_MIX_AUDIO_EN		// for HS device audio
#endif

/** RecSource + RecSink **/
//#define CFG_FUNC_RECORDER_EN

//#define CFG_FUNC_USB_HOST_AUDIO_MIX_MODE	//fs host uac, UsbHostSource +  AudioUsbHostSink
#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
	//TODO 目前FS和HS二选一,切勿同时开启。
//	#define USB_HOST_AUDIO_MIX_FS
	#define USB_HOST_AUDIO_MIX_HS
#endif

/****************************************************************************************
 *                 解码器类型选择
 *    如下解码器类型选择会影响code size;
 *****************************************************************************************/
#define USE_MP3_DECODER
#define USE_WMA_DECODER
#define USE_SBC_DECODER
#define USE_WAV_DECODER
//#define USE_FLAC_DECODER	//24bit 1.5Mbps高码率时，需要扩大DECODER_FIFO_SIZE_FOR_PLAYER 输出fifo，或扩大输入：FLAC_INPUT_BUFFER_CAPACITY
//#define USE_DTS_DECODER
//#define USE_AAC_DECODER
//#define USE_APE_DECODER
//#define USB_AIF_DECODER
//#define USB_AMR_DECODER

/*******************************************************************************************************************
 *               	 	 	 	 	 	 	 	 音效与引擎
 ******************************************************************************************************************/
#define CFG_FUNC_ROBOEFFECT_EN    //引擎使能  需下载mva或音效bin
#ifdef CFG_FUNC_ROBOEFFECT_EN

	/*双引擎模式,音效图以effect_flow_name为准@flow_config.h*/
//	#define ROBOEFFECT_DOUBLE_ENGINE_EN			//打开双图,注意MEMORY占用
	#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
		#define ROBOEFFECT_EXTRA_MEMORY2		50		//引擎2占用MEMORY = 图上已开音效MEMORY + ROBOEFFECT_EXTRA_MEMORY2
	#endif

	#define ROBOEFFECT_EXTRA_MEMORY      		50		//引擎占用MEMORY = 图上已开音效MEMORY + ROBOEFFECT_EXTRA_MEMORY

	#define MSG_EFFECTFLOW_SWITCH_EN			0		//音效图按键切换功能。key默认关闭，需要时打开该宏，key消息支持音效框图切换
    //#define CFG_FUNC_ECHO_DENOISE			//消除快速调节delay时的杂音，
 	//#define CFG_FUNC_MUSIC_EQ_EN			//Music EQ模式功能配置

//Porting#define CFG_FUNC_EFFECT_BYPASS_EN		//开启后默认运行bypass音效框图，用于音频指标测试

//	#define CFG_FUNC_MIC_ECHO_REVERB_EN		//Mic混响回声效果调节配置
//	#define CFG_FUNC_MIC_TREB_BASS_EN		//Mic高低音调节功能配置
//	#define CFG_FUNC_MUSIC_TREB_BASS_EN		//Music高低音调节功能配置

//	#define CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN					//无信号自动关机功能，
    #ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
		#define  SILENCE_THRESHOLD								120        //设置信号检测门限，小于这个值认为无信号
		#define  SILENCE_POWER_OFF_DELAY_TIME					10*60*1000 //无信号关机延时时间:10Min，单位：ms
    #endif

	#define CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN//在线调音
	#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
		//usb调音和uart调音二选一,不要同时打开
//#define CFG_COMMUNICATION_BY_HS_USB			//在线调音硬件接口HS USB HID
		#define  CFG_COMMUNICATION_BY_FS_USB			//在线调音硬件接口FS USB HID
//		#define  CFG_COMMUNICATION_BY_UART				//UART 在线调音，注意DMA资源是否足够
		#define  CFG_COMMUNICATION_CRYPTO						0////调音通讯加密=1 调音通讯不加密=0
		#define  CFG_COMMUNICATION_PASSWORD                     0x11223344//////四字节的长度密码
	#endif

	#define CFG_EFFECT_PARAMBIN_EXTRA_FLASH_SIZE				16  //KB，编译时Flash额外分配给音效bin的空间
	#define CFG_EFFECT_PARAMBIN_IN_FLASH_SIZE					64  //KB，Flash上音效音效bin的空间(只有在Flash索引表无法读取或未找到对应条目作为兜底使用)

	/*通过音效硬件参数来配置硬件参数,
	 *如果启用该宏，则系统中软件配置defult参数会失效***/
	#define CFG_FUNC_Parambin_SET_CODEC_EN
#endif

#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	/*******************************************************************
	 *  双图桥接
	 *  音效图A Set：BRIDGE_XXX_SINK ->(BRIDGE_XXX Fifo)-> BRIDGE_XXX_SOURCE:音效图B Get
	 *  BRIDGE_XXX 命名跟随图A特征+桥接数量编号
	 *  开一组宏后，双图分别存在对应Source/Sink时,才能挂载成功
	 *  New：1、拷贝bridge_xxx.c;2、拷贝条目/换通路;3、新代码换BRIDGE_XXX/BridgeXxx字符串;4、flow_config.h及音效图加通路
	 * ***************************************************************/
	#define BRIDGE_MUSIC_EN//AEC双图需开启
//	#define BRIDGE_MUSIC2_EN
//	#define BRIDGE_MIC_EN
//	#define BRIDGE_MIC2_EN

	#ifdef BRIDGE_MUSIC_EN
		#define	BRIDGE_MUSIC_SINK		BRIDGE_MUSIC_SINK_NUM
		#define BRIDGE_MUSIC_SOURCE		BRIDGE_MUSIC_SOURCE_NUM
		#define BRIDGE_MUSIC_WIDTH		CFG_DEFAULT_WIDTH
		#define BRIDGE_MUSIC_CHN		2
	//	#define BRIDGE_MUSIC_HWSRC_EN	//192K—>16K AEC图缺宏MCPS:~135M
	#endif
	#ifdef BRIDGE_MUSIC2_EN
		#define	BRIDGE_MUSIC2_SINK		BRIDGE_MUSIC2_SINK_NUM
		#define BRIDGE_MUSIC2_SOURCE	BRIDGE_MUSIC2_SOURCE_NUM
		#define BRIDGE_MUSIC2_WIDTH		CFG_DEFAULT_WIDTH
		#define BRIDGE_MUSIC2_CHN		2
	//	#define BRIDGE_MUSIC2_HWSRC_EN
	#endif
	#ifdef BRIDGE_MIC_EN
		#define	BRIDGE_MIC_SINK			BRIDGE_MIC_SINK_NUM
		#define BRIDGE_MIC_SOURCE		BRIDGE_MIC_SOURCE_NUM
		#define BRIDGE_MIC_WIDTH		CFG_DEFAULT_WIDTH
		#define BRIDGE_MIC_CHN			1
	//	#define BRIDGE_MIC_HWSRC_EN		//资源互斥 只一路HWSRC
	#endif
	#ifdef BRIDGE_MIC2_EN
		#define	BRIDGE_MIC2_SINK		BRIDGE_MIC2_SINK_NUM
		#define BRIDGE_MIC2_SOURCE		BRIDGE_MIC2_SOURCE_NUM
		#define BRIDGE_MIC2_WIDTH		CFG_DEFAULT_WIDTH
		#define BRIDGE_MIC2_CHN			1
		#define BRIDGE_MIC2_HWSRC_EN	//资源互斥 只一路HWSRC
	#endif
	#define BRIDGE_FIFO_JITTER			(2 * OSTICK_RELOAD)//uS, 缓冲时间：Flow1Step + Flow2Step + Jitter,MCPS负荷较满时扩大,免溢出丢数据
	#define ROBOEFFECT_DOUBLE_ENGINE_TWS_SWITCH_MUSIC_EN //缺省Music_KaraOK图 Tws复用,"tws_gain_mute"管理APP_SOURCE
#endif

/****************************************************************************************
 *              录音功能配置
 *   CFG_FUNC_RECORD_EXTERN_FLASH_EN	//录制多段提示音到外置flash/内部flash  用于特效音录制和播放
 *   录音介质选择内部flash时，系统MCPS要70M，比如系统超频到384M，主频不超过315M
 *   录制多段提示音到外置flash/内部flash用于特效音录制和播放，和CFG_FUNC_RECORD_SD_UDISK只能2选1
 *   关闭USE_EXTERN_FLASH_SPACE宏，录制特效音到内部flash，还需要到 \BT_Audio_APP\tools\merge_script\merge.ini配置地址
 *   merge.ini中开启如下配置，	maxlen长度需要配置为 CFG_PARA_RECORDS_MAX_SIZE * CFG_PARA_RECORDS_INDEX
 *   [rec_data]
 *   virtual = 1
 *   fullpath=..\merge_script\rec_data.bin
 *   maxlen = 0x30000
 *   enable = 1
 ****************************************************************************************/
#ifdef CFG_FUNC_RECORDER_EN
//	#define	CFG_FUNC_RECORD_EXTERN_FLASH_EN				//录音到外置flash/内部flash
	#define CFG_FUNC_RECORD_SD_UDISK					//录音到SD卡或者U盘

	#define WAV_BIT_PER_SAMPLE					16		//WAV格式采样位数16，24
	#define RECORD_FORMAT_WAV					1		//录音格式WAV
	#define RECORD_FORMAT_MP2					2		//录音为mp2格式
	#define RECORD_FORMAT_MP3					3		//录音为mp3格式

	#ifdef CFG_FUNC_RECORD_SD_UDISK
		#define CFG_FUNC_RECORD_UDISK_FIRST				//U盘和卡同时存在时，录音设备优先选择U盘，否则优先选择录音到SD卡。
		#define CFG_PARA_RECORDS_FOLDER 		"REC"	//录卡录U盘时根目录文件夹。注意ffpresearch_init 使用回调函数过滤字符串。
		#define CFG_FUNC_RECORDS_MIN_TIME		1000	//单位ms，开启此宏后，小于这个长度的自动删除。
		#define CFG_PARA_REC_MAX_FILE_NUM       256		//录音文件最大数目

		#define MEDIAPLAYER_SUPPORT_REC_FILE            // U盘或TF卡模式下，打开此功能，则支持播放录音文件；否则，只能在录音回放模式下播放录音文件
//		#define AUTO_DEL_REC_FILE_FUNCTION				//录音文件达到最大数后，自动删除全部录音文件的功能选项
//		#define MEDIAPLAYER_REC_DISK_PERF_OPTIMIZE		//磁盘写入性能优化，缺点1：需要额外的4K内存，缓存buf也需要增加一倍
	#endif

	#define DEL_REC_FILE_EN

	#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
		//#define USE_EXTERN_FLASH_SPACE								//使用外置flash空间(屏蔽这行使用芯片内置flash)

		#define CFG_PARA_RECORDS_MAX_SIZE				(3*64*1024)		// 定义为一个录音文件的大小192K,必须为64k整数倍
		#define CFG_PARA_RECORDS_INFO_SIZE				256				// 定义256BYTE空间，用来放置一些想保存的录音信息，例如录音时长、录音大小等等等
		#define	EXTERN_FLASH_RECORDER_FILE_SECOND		30				//单个文件录音时间

		#ifdef USE_EXTERN_FLASH_SPACE
			#define CFG_PARA_RECORDS_INDEX				4				// 定义最大的允许录音数量
			#define	CFG_PARA_RECORDS_FLASH_BEGIN_ADDR	0				// 录音文件的起始地址
			#define	SpiWrite(a,b,c)						SPI_Flash_Write(a,b,c)
			#define	SpiRead(a,b,c)						SPI_Flash_Read(a,b,c)
			#define SpiErase(a)							SPI_Flash_Erase_4K(a)
		#else
			#if (CFG_PARA_RECORDS_MAX_SIZE <= (2*64*1024))				// 分配空间不超过128k则直接录音前全擦
				#define CFG_PARA_RECORDS_ERASE_ALL						// 打开此定义，则在录音开始就擦除全部空间，后面只写（开始录音的时间会比较长，一个64k大概700-800ms）
			#endif
			#define CFG_PARA_RECORDS_INDEX				1				// 定义最大的允许录音数量
			#define CFG_FLASH_MUTEX_USE
			#define	SpiWrite(a,b,c)						SpiFlashWriteWpr(a,b,c,1)
			#define	SpiRead(addr,buf,len)				memcpy((void *)(buf),(void *)(addr),len)//SpiFlashReadWpr(a,b,c,1)
			#define SpiErase(a)							SpiFlashEraseWpr(BLOCK_ERASE, a, 1)
		#endif
	#endif

	/*N >= 2 ；考虑128系统帧以及加音效MIPS较高，优先级为3的编码进程处理数据较慢，推荐值为 6。提高系统帧，mips低时可以调小N,节约ram*/
	#define MEDIA_RECORDER_FIFO_N					6
	#define MEDIA_RECORDER_FIFO_LEN				(CFG_PARA_MAX_SAMPLES_PER_FRAME * MEDIA_RECORDER_CHANNEL * MEDIA_RECORDER_FIFO_N)
	/*调整下列参数后，录音介质可能需要重做兼容性测试 适配FILE_WRITE_FIFO_LEN*/
	#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
		#define MEDIA_RECORDER_CHANNEL				1
		#define MEDIA_RECORDER_BITRATE				48 //Kbps
		#define MEDIA_RECORDER_FORMAT				RECORD_FORMAT_MP2
	#else
		#define MEDIA_RECORDER_CHANNEL				2
		#define MEDIA_RECORDER_BITRATE				96 //Kbps
		#define MEDIA_RECORDER_FORMAT				RECORD_FORMAT_MP3	// RECORD_FORMAT_WAV //RECORD_FORMAT_MP3 //RECORD_FORMAT_MP2
	#endif


	/*FIFO_Len=(码率(96) / 8 * 缓冲时间ms(1000) （码率单位Kbps,等效毫秒）*/
	/*根据SDIO协议，写卡阻塞存在250*2ms阻塞 可能，实测部分U盘存在785ms周期性写入阻塞，要求编码数据fifo空间 确保超过这个长度的两倍(含同步)*/
	#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV	//录音格式WAV
		#define FILE_WRITE_FIFO_LEN					((((48*MEDIA_RECORDER_CHANNEL*(WAV_BIT_PER_SAMPLE/8)) * 200 ) / 512) * 512) //200ms
	#else
		#ifdef MEDIAPLAYER_REC_DISK_PERF_OPTIMIZE
			#define MEDIA_RECODER_IO_BLOCK_TIME			2000	//ms
			#define MEDIA_RECODER_SYNC_FILE_MS			60*1000	//ms    // 关掉此宏U盘录音的兼容性更好，但录音时掉电录音将会丢失
		#else
			#define MEDIA_RECODER_IO_BLOCK_TIME			1000	//ms
			#define MEDIA_RECODER_SYNC_FILE_MS			2000	// ms
		#endif
		#define FILE_WRITE_FIFO_LEN					((((MEDIA_RECORDER_BITRATE / 8) * MEDIA_RECODER_IO_BLOCK_TIME ) / 512) * 512)//(依据U盘/Card兼容性需求和RAM资源可选400~1500ms。按扇区512对齐
	#endif
#endif //CFG_FUNC_RECORDER_EN

/****************************************************************************************
 *                 U盘或SD卡模式相关功能配置
 *
 *****************************************************************************************/
#if(defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN) || BT_AVRCP_SONG_TRACK_INFOR)

#define PREVIEW_PLAY_TIME	10

//	#define CFG_FUNC_LRC_EN			 			// LRC歌词文件解析

	/*------browser function------*/
//	#define FUNC_BROWSER_PARALLEL_EN			// browser Parallel
//	#define FUNC_BROWSER_TREE_EN				// browser tree
	#if	defined(FUNC_BROWSER_PARALLEL_EN)||defined(FUNC_BROWSER_TREE_EN)
		#define FUNCTION_FILE_SYSTEM_REENTRY
		#if defined(FUNC_BROWSER_TREE_EN)||defined(FUNC_BROWSER_PARALLEL_EN)
			#define GUI_ROW_CNT_MAX		5		// 最多显示多少行
		#else
			#define GUI_ROW_CNT_MAX		1		// 最多显示多少行
		#endif
	#endif
	/*------browser function------*/

	/**字符编码类型转换 **/
	/**目前支持Unicode     ==> Simplified Chinese (DBCS)**/
	/**字符转换库由fatfs提供，故需要包含文件系统**/
	/**如果支持转换其他语言，需要修改fatfs配置表**/
	#define CFG_FUNC_STRING_CONVERT_EN			// 支持字符编码转换

//	#define CFG_FUNC_DECRYPT_EN					// 支持加密文件播放功能(加密文件由MVAssistant上位机工具生成)

//	#define CANCEL_COMMON_SIGNATURE_JUDGMENT	// 取消AA55判断,fatfs内磁盘系统MBR和DBR扇区结尾有此标记检测，为提高非标类型盘兼容性，可开启此项, 为有效鉴定无效盘，此项默认关闭

//	#define FUNC_UPDATE_CONTROL					// 升级交互过程控制(通过按键确认升级)
#endif


/************************************************************************************************************
 *    									转采样、时钟微调同步
 *     硬件微调是指MCLK跟随外设音源,一场景只跟随一个时钟源设备。
 *     某些外设具有同源多音频通路，可同时配硬件微调,自动适应有效音频流激活一路微调(source优先,低ID优先)
 *     非同源设备严禁同时配置和使能硬件微调,HWAdj通路ID (bit)
***********************************************************************************************************/
#define	CFG_FUNC_MIXER_SRC_EN			//MixNet统一采样率(勿关)，默认CFG_DEFAULT_SAMPLERATE,开音效后随音效图指定
#if defined(CFG_FUNC_MIXER_SRC_EN)
	/**Polyphase/Sinc必须二选一**/
//	#define	CFG_FUNC_SRC_POLYPHASE_EN	//低RAM固定  采样率少
	#define CFG_FUNC_SRC_SINC_EN		//高RAM动态 低MCPS,音质可配,init高耗

	#if defined (CFG_APP_USB_AUDIO_MODE_EN) || defined (CFG_RES_USB_HS_MIX_AUDIO_EN)
		#define	CFG_FUNC_USB_SPEAKER_HW_SRC_EN//USB下行使用硬件SRC0模块，Sam
		#ifdef CFG_FUNC_USB_SPEAKER_HW_SRC_EN
			#define CFG_USB_SPEAKER_HW_SRC_PORT 					RES_SRC_0//勿改
			#define CFG_SPEAKER_SRCOUT(MAX_IN, RATE_OUT)			((MAX_IN) * (RATE_OUT) * 11/ (44100 * 10))//MaxIsoFrame * MaxScale(System/Speaker) * 1.1
		#endif
		#define	CFG_FUNC_USB_MIC_HW_SRC_EN//USB上行使用硬件SRC1模块，Sam
		#ifdef CFG_FUNC_USB_MIC_HW_SRC_EN
			#define CFG_USB_MIC_HW_SRC_PORT 						RES_SRC_1//勿改
			#define CFG_USBMIC_SRCOUT(MAX_IN, RATE_IN, RATE_OUT)	((MAX_IN) * (RATE_OUT) * 11 / ((RATE_IN) * 10))//MaxFrame * MaxScale (UsbMic/System) * 1.1
		#endif
	#endif
	#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
		#define CFG_BRIDGE_HW_SRC_PORT 								RES_SRC_2//勿改
	#endif
#endif

//#define	CFG_FUNC_AUDIO_SILENCE_AVOIDANCE_EN		//DAC防播空机制

#define	CFG_FUNC_MCLK_ADJUST					//硬件微调
#ifdef CFG_FUNC_MCLK_ADJUST
	#define	CFG_FUNC_USB_MCLK_ADJUST_EN
//	#define CFG_PARA_BT_FREQ_ADJUST		//Btplay 模式续存期间 硬件微调，与CFG_PARA_BT_SYNC宏配合
//	#define CFG_PARA_HFP_FREQ_ADJUST	//通话模式 续存期间 硬件微调  使用上行微调，下行跟随。 与CFG_PARA_HFP_SYNC配合
#endif

/************************************************************************************************************
 *    									外部时钟灌入、音频时钟同步
 *     外部时钟灌入是指启用外部I2S BLCK、SPDIF BLCK时钟进行时钟同步功能
***********************************************************************************************************/
#if defined(CFG_APP_I2SIN_MODE_EN) || defined(CFG_I2S0_MIX_IN_EN) || defined(CFG_I2S0_MIX_OUT_EN) || defined(CFG_I2S1_MIX_IN_EN) || defined(CFG_I2S1_MIX_OUT_EN) || defined(CFG_I2S2_MIX_IN_EN) || defined(CFG_I2S2_MIX_OUT_EN)
//#define CFG_FUNC_I2S_DPLL_POLICY_EN        //DPLL0外部时钟启用I2S BLCK外部时钟跟随功能
#endif 

/************************************************************************************************************
 *                					LOG DEBUG功能配置
 *		SHELL功能需要开启 UART DEBUG功能
 *************************************************************************************************************/
#define CFG_FUNC_DEBUG_EN                       /* UART1 boot log; protocol takes over after startup */
//#define CFG_FUNC_USBDEBUG_EN                  // USB日志关闭，FS USB仅保留升级链路
#ifdef CFG_FUNC_DEBUG_EN
	#define CFG_UART_LOG_EN						//端口等移gpio_config.h
	#ifdef CFG_UART_LOG_EN
		#define CFG_FUNC_DEBUG_USE_TIMER		//使用定时器进行打印
	#endif
	//#define CFG_FUNC_PERIODIC_PERF_LOG_EN		//周期CPU/Frame/RAM日志，协议版本默认关闭
	#define CFG_DEBUG_FRAME_PAYLOAD				//音频帧负载统计
//	#define	CFG_DEBUG_DAC_EMPTY_EN
#endif

/*core2 UART 打印端口配置，默认NULL，不开启*/
/*core2的打印口目前只配置为UART2这组*/
#ifdef DUAL_CORE_EN
#define CORE2_UART			UART2_NULL//core UART 端口配置
//#define CORE2_SW_PORT_EN	//GPIO A0和A1 设置为SW复用功能，默认设置为普通端口
#endif

/****************************************************************************************
 *                 提示音功能配置
 *    1.烧录工具参见MVs26_SDK\tools\script；
 *    2.提示音功能开启，注意flash中const data提示音数据需要预先烧录，否则不会播放;
 *    3.const data数据开机检查，影响开机速度，主要用于验证。
 *****************************************************************************************/
#ifdef CFG_FUNC_REMIND_SOUND_EN
//	#define CFG_REMIND_SOUND_DECODING_USE_LIBRARY	//双解码播放MP3提示音,音质更好,资源消耗更多,打开此宏须用\B6_Audio_SDK\tools下的script_MP3.bat替换掉原来的script.bat
	#define CFG_PARAM_FIXED_REMIND_VOL   	CFG_PARA_SYS_VOLUME_DEFAULT/2		//固定提示音音量值,0表示受music vol同步控制
#endif

/****************************************************************************************
 *                 低功耗模式配置
 *IDLE模式(假待机),powerkey/deepsleep可以同时选中也可以单独配置
 *通过消息进入不同的模式
 *MSG_DEEPSLEEP/MSG_POWER/MSG_POWERDOWN --> 进入IDLE模式(假待机)
 *MSG_DEEPSLEEP --> 进入IDLE模式以后如果CFG_IDLE_MODE_DEEP_SLEEP打开进入deepsleep
 *MSG_POWERDOWN --> 进入IDLE模式以后如果CFG_IDLE_MODE_POWER_KEY打开进入powerdown
 *****************************************************************************************/
#ifdef  CFG_APP_IDLE_MODE_EN
	#define CFG_IDLE_MODE_DEEP_SLEEP //deepsleep，500uA~1mA
	#define CFG_IDLE_MODE_POWER_KEY //PowerDown，4uA
	#ifdef CFG_IDLE_MODE_POWER_KEY
		#define POWERKEY_MODE		POWERKEY_MODE_PUSH_BUTTON
//		#if(POWERKEY_MODE==POWERKEY_MODE_PUSH_BUTTON)
//			//#define POWERKEY_FIRST_ENTER_POWERDOWN		//第一次上电需要按下PowerKey
//		#endif
//		#define POWER_UP_GPIOC
		#ifdef POWER_UP_GPIOC
			#define GPIOC_POWERON_PIN	GPIOC_POWERUP_PIN_C0
			#define GPIOC_PIN_MODE		GPIOC_POWERUP_TYPE_RISING_EDGE
		#endif
	#endif
	#ifdef CFG_IDLE_MODE_DEEP_SLEEP
		/*红外按键唤醒,注意CFG_PARA_WAKEUP_GPIO_IR和 唤醒键IR_KEY_POWER设置*/
		#define CFG_PARA_WAKEUP_SOURCE_IR		SYSWAKEUP_SOURCE10_IR//固定source11
		/*ADCKey唤醒 配合CFG_PARA_WAKEUP_GPIO_ADCKEY 唤醒键ADCKEYWAKEUP设置及其电平*/
		#define CFG_PARA_WAKEUP_SOURCE_ADCKEY	SYSWAKEUP_SOURCE1_GPIO//如使用ADC唤醒，必须打开CFG_RES_ADC_KEY_SCAN 和CFG_RES_ADC_KEY_USE
//		#define CFG_PARA_WAKEUP_SOURCE_CEC		SYSWAKEUP_SOURCE2_GPIO//HDMI专用，CFG_PARA_WAKEUP_GPIO_CEC
		#define CFG_PARA_WAKEUP_SOURCE_POWERKEY SYSWAKEUP_SOURCE6_POWERKEY
		#define CFG_PARA_WAKEUP_SOURCE_IOKEY1	SYSWAKEUP_SOURCE3_GPIO
		#define CFG_PARA_WAKEUP_SOURCE_IOKEY2	SYSWAKEUP_SOURCE4_GPIO
	#endif
#endif

#if 0
	#define LOW_POWER_TEST
	#ifdef LOW_POWER_TEST
		#define BT_CONNECTED_DEEPSLEEP
		// #define DEEPSLEEP_IO_CONFIG

		#define CONFIG_V
		#define RC12M_TRIM
		#define LOG_ANALYSIS_DBG
			#ifdef LOG_ANALYSIS_DBG
				// #define LOG_ANALYSIS_DBG_EXT
				// #define LOG_ANALYSIS_DBG_SWITCH
				// #define LOG_ANALYSIS_DBG_SWITCH_2
			#endif

		#define SUSPEND_RESUME
		#define EXTERNAL_TRIGGER
		// #define PAGE_ENTER_DEEPSLEEP
		#define SNIFF_WDG_FEED
	#endif
#endif


/****************************************************************************************
 *                 断点记忆功能配置
****************************************************************************************/
#define CFG_FUNC_BREAKPOINT_EN
#ifdef CFG_FUNC_BREAKPOINT_EN
	#define BP_PART_SAVE_TO_NVM			// 断点信息保存到NVM
	#define BP_SAVE_TO_FLASH			// 断电信息保存到Flash
	#define FUNC_MATCH_PLAYER_BP		// 获取FS扫描后与播放模式断点信息匹配的文件。文件夹和ID号
#endif


/**********************************************************************************************************
 *                           	   	   	   Key 按键相关配置
 ***********************************************************************************************************/
/**按键beep音功能**/
/**需要触发的按键位置添加BeepEnable();API */
//#define CFG_FUNC_BEEP_EN
	#define CFG_PARA_BEEP_DEFAULT_VOLUME	15//注意:若蓝牙音量同步功能开启后，此值最大为16

/**按键双击功能**/
//#define CFG_FUNC_DBCLICK_MSG_EN

/**ADC按键**/
#define CFG_RES_ADC_KEY_SCAN

/**IR按键**/
//#define CFG_RES_IR_KEY_SCAN				//启用device service Key扫描IRKey

/**编码旋钮按键**/
//#define CFG_RES_CODE_KEY_USE

/**GPIO按键**/
//#define CFG_RES_IO_KEY_SCAN

/**电位器功能选择**/
//#define CFG_ADC_LEVEL_KEY_EN

/**电池电量监测**/
//#define CFG_FUNC_POWER_MONITOR_EN

/**DMA RGB功能**/
//#define CFG_DMA_RGB_LED_EN

/***************************************************************************************
*					RTC/闹钟功能配置
*    OSC_32K: RTC时钟选择晶体晶振32.768kHZ
*    OSC_24M: RTC时钟选择晶体晶振24MHZ
***************************************************************************************/
//#define CFG_FUNC_RTC_EN
#ifdef CFG_FUNC_RTC_EN
	#define CFG_FUNC_RTC_OSC_FREQ		OSC_32K
	#define CFG_FUNC_ALARM_EN  			//闹钟功能,必须开时钟
	#define CFG_FUNC_LUNAR_EN  			//万年历,必须开时钟
	#ifdef CFG_FUNC_ALARM_EN
		#define CFG_FUNC_SNOOZE_EN 		//闹钟贪睡功能
	#endif
#endif

/****************************************************************************************
 *                           Display 显示配置
****************************************************************************************/
//#define  CFG_FUNC_DISPLAY_EN
#ifdef CFG_FUNC_DISPLAY_EN
//	#define  DISP_DEV_SLED
	#define  DISP_DEV_7_LED
#endif

/**********************************************************************************************************
 *                           	   	   	   SarADC功能使能
 ***********************************************************************************************************/
#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_ADC_LEVEL_KEY_EN)
	#define CFG_RES_SarADC_USE
#endif


#ifndef CFG_COMMUNICATION_BY_UART
//Porting #define CFG_UPGRADE_BY_UART
#endif

/**********************************************************************************************************
 *                           	   	   	   USB功能相关配置
 ***********************************************************************************************************/
#if defined(CFG_APP_USB_AUDIO_MODE_EN) || defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
	#define CFG_UAC_DEVICE_HS_EN
	#define CFG_PARA_USB_MODE	 		AUDIO_MIC//READER	//USB HS只可配置 FS_DESC_BASE_OFFSET 之前的模式,如果HOST设备是FS,会自动切换到FS相对应模式。
	#define CFG_FUNC_USB_PROTOCOL_AUTO						//HOST设备不支持UAC2.0时自动切换至UAC1.0
#endif
#if defined(CFG_APP_USB_FS_AUDIO_MODE_EN) || defined(CFG_RES_USB_FS_MIX_AUDIO_EN)
	#define CFG_UAC_DEVICE_FS_EN
	#define CFG_PARA_USB_FS_MODE	 	FS_AUDIO_MIC_10		//USB FS只可配置 FS_DESC_BASE_OFFSET 之后的模式
#endif
//	#define CFG_FUNC_USB_TYPEC_HEADSET_CONFIG 	//TYPEC 耳机兼容性配置,Ctrl(无上下曲等)
	/* 还需要修改以下配置
	 * #define HID_DATA_FUN_EN		0				//关闭在线调音/PC升级
	 * #define CFG_PARA_USB_MODE	 AUDIO_MIC_10	//部分手机为了更好的兼容性还需要使用UAC1.0模式
	 * */
#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_FUNC_RECORD_SD_UDISK)
	#define UDISK_BULK_TIMEOUT				10000//ms 3000~10000可配,兼容性与加快读盘异常退出成反比
#endif

#if defined(CFG_UAC_DEVICE_HS_EN)
	#define CFG_PARA_AUDIO_USB_IN_SYNC	//时钟偏差引起的 采样点同步

	#define CFG_PARA_AUDIO_USB_OUT_SYNC	//时钟偏差引起的 采样点同步
	#define CFG_RES_AUDIO_USB_VOL_SET_EN

	#if (CFG_PARA_USB_MODE == MIC_ONLY)	|| (CFG_PARA_USB_MODE == MIC_ONLY_10)
		#define	CFG_OTG_MODE_MIC_EN			//OTG声卡 MIC开启
		#define CFG_DESCRIP_MIC_ONLY_EN
	#elif (CFG_PARA_USB_MODE == AUDIO_ONLY)	|| (CFG_PARA_USB_MODE == AUDIO_ONLY_10)
		#define	CFG_OTG_MODE_AUDIO_EN		//OTG声卡 AUDIO开启
		#define CFG_DESCRIP_AUDIO_ONLY_EN
	#elif (CFG_PARA_USB_MODE == AUDIO_MIC) || (CFG_PARA_USB_MODE == AUDIO_MIC_10)
		#define	CFG_OTG_MODE_MIC_EN
		#define	CFG_OTG_MODE_AUDIO_EN
		#define CFG_DESCRIP_AUDIO_MIC_EN
	#elif (CFG_PARA_USB_MODE == READER)
		#define	CFG_OTG_MODE_READER_EN		//OTG声卡 READER开启
		#define CFG_DESCRIP_READER_EN
	#elif (CFG_PARA_USB_MODE == AUDIO_MIC_AUDIO) || (CFG_PARA_USB_MODE == AUDIO_MIC_AUDIO_10)
		#define	CFG_OTG_MODE_MIC_EN
		#define	CFG_OTG_MODE_AUDIO_EN
		#define	CFG_OTG_MODE_AUDIO1_EN		//OTG声卡 AUDIO1开启
		#define CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	#endif
#endif

#if defined(CFG_UAC_DEVICE_FS_EN)
	#define CFG_PARA_AUDIO_USB_FS_IN_SYNC	//时钟偏差引起的 采样点同步

	#define CFG_PARA_AUDIO_USB_FS_OUT_SYNC	//时钟偏差引起的 采样点同步
	#define CFG_RES_AUDIO_USB_FS_VOL_SET_EN

	#if(CFG_PARA_USB_FS_MODE == FS_MIC_ONLY) || (CFG_PARA_USB_FS_MODE == FS_MIC_ONLY_10)
		#define	CFG_OTG_FS_MODE_MIC_EN			//OTG声卡 MIC开启
		#define CFG_DESCRIP_MIC_ONLY_EN
	#elif (CFG_PARA_USB_FS_MODE == FS_AUDIO_ONLY) || (CFG_PARA_USB_FS_MODE == FS_AUDIO_ONLY_10)
		#define	CFG_OTG_FS_MODE_AUDIO_EN		//OTG声卡 AUDIO开启
		#define CFG_DESCRIP_AUDIO_ONLY_EN
	#elif (CFG_PARA_USB_FS_MODE == FS_AUDIO_MIC) || (CFG_PARA_USB_FS_MODE == FS_AUDIO_MIC_10)
		#define	CFG_OTG_FS_MODE_MIC_EN
		#define	CFG_OTG_FS_MODE_AUDIO_EN
		#define CFG_DESCRIP_AUDIO_MIC_EN
	#elif (CFG_PARA_USB_FS_MODE == FS_READER)
		#define	CFG_OTG_FS_MODE_READER_EN		//OTG声卡 READER开启
		#define CFG_DESCRIP_READER_EN
	#elif (CFG_PARA_USB_FS_MODE == FS_AUDIO_MIC_AUDIO) || (CFG_PARA_USB_FS_MODE == FS_AUDIO_MIC_AUDIO_10)
		#define	CFG_OTG_FS_MODE_MIC_EN
		#define	CFG_OTG_FS_MODE_AUDIO_EN
		#define	CFG_OTG_FS_MODE_AUDIO1_EN		//OTG声卡 AUDIO1开启
		#define CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	#endif
#endif

#if defined(CFG_COMMUNICATION_BY_HS_USB) || defined(CFG_COMMUNICATION_BY_FS_USB)
	#define CFG_DESCRIP_HID_EN
	#if defined(CFG_COMMUNICATION_BY_HS_USB)
		#define CFG_DESCRIP_HID_HEART_BEAT_EN	//仅HS单HID使用心跳包，FS HID不依赖HS音频实现
	#endif
#endif

/************************************************************************************************************
 *dump工具,可以将数据发送到dump工具,用于分析
*************************************************************************************************************/
//Porting #define CFG_DUMP_DEBUG_EN


/**********************************************************************************************************
 * 									SDK版本号
 * 0xB6：芯片B16X，01：大版本号， 00：小版本号， 00：用户修订号（由用户设定，可结合补丁号）；
 * 开flash_boot，升级usercode后，boot明码区和code明码(如0xB8和0xB8+0x10000)两个值会不同，
 * 前者是burner烧录时版本，后者是mva版本需关注
 * *******************************************************************************************************/
#define	 CFG_SDK_VER_CHIPID			(0xB6)

#define  CFG_SDK_MAJOR_VERSION		(0)
#define  CFG_SDK_MINOR_VERSION		(3)
#define  CFG_SDK_PATCH_VERSION	    (16)

/**********************************************************************************************************
 * 									用户修改版本号
 * *******************************************************************************************************/
#define	 CFG_USE_SDK_VERSION		(0x55)//用户项目号

#define  CFG_USE_SDK_MAJOR_VERSION	(10)
#define  CFG_USE_SDK_MINOR_VERSION	(10)
#define  CFG_USE_SDK_PATCH_VERSION	(10)

/**********************************************************************************************************
 * 									DCDC低功耗配置
 * *******************************************************************************************************/
#ifdef 	CHIP_PIN_DCDC11
	#define 	CFG_DCDC11_EN //芯片内置DCDC使能,关宏提射频性能
#endif
#ifdef 	CHIP_PIN_DCDC16
	#define 	CFG_DCDC16_EN //芯片内置DCDC使能,关宏提射频性能
#endif

#define CFG_FUNC_SYSTEM_LOW_POWER

/**********************************************************************************************************
 * 									flash code区域加密
 * *******************************************************************************************************/
#define CFG_DBUS_ACCESS_EN//默认打开，不要关闭

#define CFG_FUNC_FLASH_PARAM_ONLINE_TUNING_EN//在线参数修改，CRC校验值获取

//#define SOFT_WACTH_DOG_ENABLE//启用软件看门狗监控task，task阻塞的时候进行复位

/**********************************************************************************************************
 *OS操作系统进入IDLE时经core进入休眠状态，以达到降低功耗目的
 *注意，这是OS调度的IDLE，并非应用层APPMODE，应用层无需关心
 * *******************************************************************************************************/
#define CFG_FUNC_IDLE_TASK_LOW_POWER
#ifdef	CFG_FUNC_IDLE_TASK_LOW_POWER
	#define	CFG_GOTO_SLEEP_USE
#endif

#ifdef DUAL_CORE_EN
	#define CFG_FUNC_EFFECT_SERVICE_EN //独立并出让给适配层,帧节拍高效
	#if defined(CFG_FUNC_EFFECT_SERVICE_EN) && defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
		#define CFG_FUNC_EFFECT_CORE_DUAL_EN	//双Core各自音效, Core2仅SeparateNet/Flow2
	#endif
#endif

//#define CFG_FUNC_BT_OTA_EN
/*缺省Host FS模块*/
#define OTG_HOST_PORT_FS
/*缺省Device为 HS模块,开下述宏指向FS*/
#define OTG_DEVICE_PORT_FS
#ifndef CHIP_PIN_HSOTG
	#define OTG_DEVICE_PORT_FS
#endif


#include "flash_boot.h"
#include "gpio_config.h" //gpio在前,据封装User调配pin
#include <module_config.h> //module常规勿调配
#endif /* _APP_CONFIG_H_ */
