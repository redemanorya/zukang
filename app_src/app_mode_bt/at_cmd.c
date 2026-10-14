

#include <stdlib.h>
#include <string.h>
#include "le_audio_config.h"
#if BT_LE_AUDIO_SUPPORT
#include "le_bis_sink.h"
#include "le_bis_src.h"
#include "debug.h"
#include "app_le_call_client.h"
#include "app_le_media_server.h"
#include "app_le_call_server.h"
#include "app_le_volume_controller.h"

// static void (*rxchar)(uint8_t) = NULL;
extern void app_serialRx(uint8_t c);
extern uint8_t bcast_code[16];

// 函数声明
extern uint8_t check_bis_src_is_in_idle(void);

#define MAX_HOST_CMD_LEN 64
uint8_t cmd_idx = 0;
uint8_t parse_idx = 0xFF;
uint8_t	at_buf[MAX_HOST_CMD_LEN + 1];
uint8_t app_rx_state = 0;
uint8_t switch_src_flag = 0xFF;

typedef struct {
    uint8_t cmd0;
    uint8_t cmd1;
    uint16_t min_len;
    void (*handler)(uint8_t *buf, uint16_t len);
} HidCmdEntry;

extern uint8_t target_deleg_name[30];
extern uint8_t g_bcast_id[3];
extern void le_bis_assis_start_scan(void);

extern void app_le_client_media_play(void);
extern void app_le_client_media_pause(void);
extern void app_le_client_media_stop(void);
extern void app_le_client_media_previous(void);
extern void app_le_client_media_next(void);

extern void app_le_vcs_volume_increase(void);
extern void app_le_vcs_volume_decrease(void);

unsigned char char_to_hex(char c) {
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }   
    else if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }     
    else if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    else {
      APP_DBG("input code error\r\n");
      return 0;  // Return default value for invalid input
    }
}

// bcast_code
static void handle_cmd_AA(uint8_t *buf, uint16_t len) 
{
    uint8_t bcast_code[16] = {0};
    APP_DBG("hid recv = AT#AA\n");
    memcpy(bcast_code, buf+2, 4);
    for (int i = 0; i < 4; i++) {
        APP_DBG("bcast_code %d, 0x%x\n", i, bcast_code[i]);
    }
#ifdef LE_BIS_ASSIS
    le_bis_assis_set_bcast_code(bcast_code);
#else
    set_phone_bcast_code(bcast_code);
#endif
    le_bis_source_set_bcast_code(bcast_code);
}

static void handle_cmd_AB(uint8_t *buf, uint16_t len) {
    APP_DBG("hid recv = AT#AB\n");
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
    switch (buf[2])
    {
        case 'A':
        {
            app_le_client_media_play();
        } break;

        case 'B':
        {
            app_le_client_media_pause();
        } break;

        case 'C':
        {
            app_le_client_media_stop();
        } break;

        case 'D':
        {
            app_le_client_media_previous();
        } break;

        case 'E':
        {
            app_le_client_media_next();
        } break;

        default:
            break;
    }
#endif
}

static void handle_cmd_AC(uint8_t *buf, uint16_t len) 
{
	APP_DBG("hid recv = AT#AC\n");

	if (buf[2] == '0')
	{
		Set_local_src(0);
#ifdef LE_BIS_ASSIS
		le_bis_assis_start_scan();
#endif
	}
	else if (buf[2] == '1')
	{
		Set_local_src(1);
#ifdef LE_BIS_ASSIS
		le_bis_assis_start_scan();
#endif
	}
}

static void handle_cmd_AD(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AD\n");

    uint8_t input[6]={0};

    if(len < 8 )
    {
        return;
    }

    for(int  i = 0; i< 6; i++)
    {
        input[i] = buf[i+2];
    }
    APP_DBG("broad_cast_code:");

    for (int i = 0; i < 6; i += 2) 
    {
        unsigned char high = char_to_hex(input[i]);
        unsigned char low = char_to_hex(input[i+1]);
        
        unsigned char byte = (high << 4) | low;

		g_bcast_id[i/2] = (uint8_t)byte;
        APP_DBG("0x%02X", byte);  
    }
}

static void handle_cmd_AE(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AE src state 0x%x\n", check_bis_src_is_in_idle());
    
    if (check_bis_src_is_in_idle())
    {
        set_src_wed_vaild(buf[2] - '0');
    }
}

static void handle_cmd_AF(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AF\n");
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
    switch (buf[2])
    {
        case 'A':
        {
            app_le_vcs_volume_increase();
        } break;

        case 'B':
        {
            app_le_vcs_volume_decrease();
        } break;

        default:
            break;
    }
#endif
}

static void handle_cmd_AG(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AG\n");
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
    switch (buf[2])
    {
        case 'A':
        {
            app_vcc_volume_increase();
        } break;

        case 'B':
        {
            app_vcc_volume_decrease();
        } break;

        default:
            break;
    }
#endif
}

static void handle_cmd_AH(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AH\n");
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
	if ((buf[2] > '4') || (buf[2] < '1'))
	{
		APP_DBG("err cis_app 0x%x\n", buf[2]);
		return;
	}

	if (buf[3] > '2')
	{
		APP_DBG("err sink_loc 0x%x\n", buf[3]);
		return;
	}

	uint8_t cis_app = (uint8_t)(buf[2] - '0');
	uint8_t sink_loc = (uint8_t)(buf[3] - '0');

	APP_DBG("cis_app 0x%x, sink_loc: 0x%x\n", cis_app, sink_loc);
#if defined(LE_CIS_SRC) || defined(LE_CIS_SINK)
	cis_app_init(cis_app, sink_loc);
#endif
#endif
}


static void handle_cmd_AI(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AI: src_idx %d\n", (uint8_t)(buf[2] - '0'));
    uint8_t src_idx = (uint8_t)(buf[2] - '0');
#ifdef SINK_CHOOSE_SRC
    app_add_src_for_sink(src_idx);
#endif
}


static void handle_cmd_AJ(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AJ\n");
}

static void handle_cmd_AK(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AK\n");
}

extern void unicast_client_start_cfg(uint8_t scene);
static void handle_cmd_AL(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AL\n");
}

static void handle_cmd_AM(uint8_t *buf, uint16_t len) 
{
    APP_DBG("hid recv = AT#AM: assis\n");
    switch (buf[2])
    {
#ifdef LE_BIS_ASSIS
        case 'A':
        {
            app_broadcast_assistant_add_src(buf[3] - '0');
        } break;

        case 'B':
        {
            app_broadcast_assistant_modify_source(buf[3] - '0', buf[4] - '0');
        } break;

        case 'C':
        {
            app_bis_assistant_remove_src();
        } break;

        case 'D':
        {
            uint8_t target_src_idx = buf[3] - '0';
            app_bis_assistant_switch_src(target_src_idx);
            switch_src_flag = 1;
        } break;

        case 'E':
        {

        } break;

        default:
            break;
#endif
    }
}

static const HidCmdEntry hid_cmd_table[] = {
    { 'A', 'A', 3, handle_cmd_AA },
    { 'A', 'B', 3, handle_cmd_AB },
    { 'A', 'C', 3, handle_cmd_AC },
    { 'A', 'D', 3, handle_cmd_AD },
    { 'A', 'E', 3, handle_cmd_AE },
    { 'A', 'F', 3, handle_cmd_AF },
    { 'A', 'G', 3, handle_cmd_AG },
    { 'A', 'H', 3, handle_cmd_AH },
    { 'A', 'I', 3, handle_cmd_AI },
    { 'A', 'J', 3, handle_cmd_AJ },
    { 'A', 'K', 3, handle_cmd_AK },
    { 'A', 'L', 3, handle_cmd_AL },
    { 'A', 'M', 3, handle_cmd_AM },
};

void hidParseHostCmd(uint8_t* buf, uint16_t len)
{
    if (!buf || len < 2) return;
    for (size_t i = 0; i < sizeof(hid_cmd_table)/sizeof(hid_cmd_table[0]); ++i) {
        if (buf[0] == hid_cmd_table[i].cmd0 && buf[1] == hid_cmd_table[i].cmd1) {
            if (len >= hid_cmd_table[i].min_len) {
                hid_cmd_table[i].handler(buf, len);
            }
            return;
        }
    }
}
#endif
