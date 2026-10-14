/**
 **************************************************************************************
 * @file    roboeffect_prot.h
 * @brief   a state machine driven uart protocol parser
 *
 * @author  Castle Cai
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */


#ifndef _TLV_H_
#define _TLV_H_

#include <stdio.h>
#include "type.h"
#include "mvstdio.h"


#define ROBOEFFECT_HUGE_BUFFER_SIZE 16384
#define ROBOEFFECT_BIG_BUFFER_SIZE 4096
#define ROBOEFFECT_MEDIUM_BUFFER_SIZE 2048
#define ROBOEFFECT_SMALL_BUFFER_SIZE 1024
#define ROBOEFFECT_PARSE_MAX_SIZE 512

#define PROTOCOL_BODY_LEN 1024
#define ROBOEFFECT_PORT_MAX 1
/*for tcp server protocol parser*/
typedef enum _roboeffect_prot_parsing_state
{
	ROBOEFFECT_PROT_PARSING_STATE_NONE,
	ROBOEFFECT_PROT_PARSING_STATE_HEAD1,
	ROBOEFFECT_PROT_PARSING_STATE_HEAD2,
	ROBOEFFECT_PROT_PARSING_STATE_LEN,
	ROBOEFFECT_PROT_PARSING_STATE_CMD,
	ROBOEFFECT_PROT_PARSING_STATE_DATABODY,
	ROBOEFFECT_PROT_PARSING_STATE_END,
	ROBOEFFECT_PROT_PARSING_STATE_FINISH
} roboeffect_prot_parsing_state;

typedef enum _roboeffect_prot_handle_state
{
	ROBOEFFECT_PROT_HANDLE_STATE_INIT,
	ROBOEFFECT_PROT_HANDLE_STATE_EXEC,
	ROBOEFFECT_PROT_HANDLE_STATE_DONE
} roboeffect_prot_handle_state;

typedef enum _roboeffect_prot_cmd_code
{
	/*request*/
	ROBOEFFECT_PROT_CMD_1 = 0,
	ROBOEFFECT_PROT_CMD_2,

	ROBOEFFECT_PROT_CMD_MAX_COUNT
} roboeffect_prot_cmd_code;

typedef struct _roboeffect_prot_parse
{
	roboeffect_prot_parsing_state state;
	MemHandle *buf;
	uint16_t start_pos;
	uint16_t cur_pos;
	uint16_t inner_pos;
	uint16_t parity;
	uint16_t data_pos;
	uint16_t datalen;
	uint8_t t_id[8];
	uint16_t t_cmd;
	uint16_t timerid;

	uint8_t raw_data[PROTOCOL_BODY_LEN/4];
} roboeffect_prot_parse;

typedef struct _roboeffect_prot_parse_context
{
	roboeffect_prot_parse parse_struct[ROBOEFFECT_PORT_MAX];
	uint8_t tmp_buf[ROBOEFFECT_MEDIUM_BUFFER_SIZE];
} roboeffect_prot_parse_context;


void roboeffect_prot_init(void);
uint16_t crc16(uint8_t d[], int32_t len);

void roboeffect_prot_reset_parse(roboeffect_prot_parse *parse);
void __roboeffect_prot_parse_package(roboeffect_prot_parse *parse, uint8_t *buf, uint16_t len);
void roboeffect_prot_parse_package_outside(uint8_t *data, uint16_t length);
void roboeffect_prot_handle_queue(void);
void roboeffect_prot_parse_big_block(uint8_t *data, uint16_t length);


#endif /* __TLV_H__ */
