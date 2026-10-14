/**
 **************************************************************************************
 * @file    roboeffect_prot.c
 * @brief   a state machine driven uart protocol parser
 *
 * @author  Castle Cai
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app_config.h"
#include "roboeffect_prot.h"


#define	ROBOEFFECT_PROT_DEBUG(format, ...)			printf(format, ##__VA_ARGS__)
#define	ROBOEFFECT_PROT_TRACE(format, ...)			printf(format, ##__VA_ARGS__)
#define	ROBOEFFECT_PROT_TRACE_TCP(format, ...)			printf(format, ##__VA_ARGS__)

extern void Communication_Effect_Config(uint8_t Control, uint8_t *buf, uint32_t len);
#if defined(CFG_FUNC_ROBOEFFECT_EN)
/*global value*/
roboeffect_prot_parse g_roboeffect_prot_parse;
uint8_t prot_tmp_buf[ROBOEFFECT_SMALL_BUFFER_SIZE] = {0}; /*buffer to hold un-processed data*/

uint8_t uart_parse_buffer[ROBOEFFECT_SMALL_BUFFER_SIZE] = {0};/*buffer to hold un-processed data with ring buffer*/

uint16_t crc16(uint8_t d[], int32_t len)
{
	uint8_t b = 0;
	uint16_t crc = 0xffff;
	int32_t i, j;

	for(i = 0; i < len; i++)
	{
		for(j = 0; j < 8; j++)
		{
			b = ((d[i] << j) & 0x80) ^ ((crc & 0x8000) >> 8);
			crc <<= 1;
			if(b != 0)
				crc ^= 0x1021;
		}
	}

	return crc;
}

/*GPRS protocol parser*/
void roboeffect_prot_reset_parse(roboeffect_prot_parse *parse)
{
	parse->state = ROBOEFFECT_PROT_PARSING_STATE_NONE;
	parse->parity = 0;
	parse->datalen = 0;
	parse->inner_pos = 0;
	parse->start_pos = 0;
	parse->cur_pos = 0;
	parse->data_pos = 0;

	parse->t_cmd = 0x00;
}

void __roboeffect_prot_parse_package(roboeffect_prot_parse *parse, uint8_t *buf, uint16_t len)
{
	uint16_t i, eat_len, restart_pos = parse->start_pos + 1;
	for (i = parse->cur_pos; i < len; i++)
	{
		if(parse->state == ROBOEFFECT_PROT_PARSING_STATE_FINISH) break;

		switch(parse->state)
		{
		case ROBOEFFECT_PROT_PARSING_STATE_NONE:
			if (0xA5 == buf[i])
			{
				parse->inner_pos = 0;
				restart_pos = parse->start_pos + 1; //we should update restart_pos every beginning
				parse->state = ROBOEFFECT_PROT_PARSING_STATE_HEAD1;
				//PROTOCOL_DEBUG("Get head1\n");
			}
			else
			{
				parse->start_pos++;
				restart_pos = parse->start_pos + 1;
			}
			break;

		case ROBOEFFECT_PROT_PARSING_STATE_HEAD1:
		case ROBOEFFECT_PROT_PARSING_STATE_HEAD2:
			if (0x5A == buf[i])
			{
				parse->inner_pos = 0;
				parse->state = ROBOEFFECT_PROT_PARSING_STATE_CMD;
				//PROTOCOL_DEBUG("Get head2\n");
			}
			else
			{
				/*go back, just pass the first start_mark byte*/
				roboeffect_prot_reset_parse(parse);
				//restart_pos++;//puzzle
				parse->start_pos = restart_pos;
				i = restart_pos - 1;
			}
			break;

		case ROBOEFFECT_PROT_PARSING_STATE_CMD:
			parse->t_cmd = buf[i];
			parse->inner_pos = 0;
			parse->state = ROBOEFFECT_PROT_PARSING_STATE_LEN;
			//PROTOCOL_DEBUG("Get command: %X\n", buf[i]);
			break;

		case ROBOEFFECT_PROT_PARSING_STATE_LEN:
			parse->inner_pos++;
			if (parse->inner_pos == 1)//len-data length
			{
				parse->datalen = buf[i];
				//if(parse->datalen && parse->datalen < 1024)
				if(parse->datalen)
				{
					//PROTOCOL_DEBUG("Data len = %X\n", parse->datalen);
					parse->state = ROBOEFFECT_PROT_PARSING_STATE_DATABODY;
					parse->data_pos = i + 1;//data is after len
				}
				else
				{	//support data len = 0
					parse->state = ROBOEFFECT_PROT_PARSING_STATE_END;
				}
				parse->inner_pos = 0;//anyway, reset inner
			}
			break;

		case ROBOEFFECT_PROT_PARSING_STATE_DATABODY:
			parse->inner_pos++;

			/*when we reach the end of raw data frame*/
			if(parse->inner_pos == parse->datalen)
			{
				/*the raw data, so do not check*/
				parse->state = ROBOEFFECT_PROT_PARSING_STATE_END;
				//smaller than PROTOCOL_BODY_LEN
				memset((void*)parse->raw_data, 0x00, sizeof(parse->raw_data));
				memcpy((void*)parse->raw_data, &buf[parse->data_pos], parse->datalen);
				parse->inner_pos = 0;
			}
			break;

		case ROBOEFFECT_PROT_PARSING_STATE_END:
			/*check the ending character*/
			if (buf[i] == 0x16)
			{
				parse->state = ROBOEFFECT_PROT_PARSING_STATE_FINISH;
				//ROBOEFFECT_PROT_DEBUG("Get one valid package: cmd=%X, data_len=%d\n", parse->t_cmd, parse->datalen);
				Communication_Effect_Config(parse->t_cmd, parse->raw_data, parse->datalen);
			}
			else
			{
				/* go back, just pass the first start_mark byte */
				roboeffect_prot_reset_parse(parse);
				parse->start_pos = restart_pos;
				i = restart_pos - 1;
			}
			break;

		default:
			break;
		}
	}/*big for end*/

	if (parse->state == ROBOEFFECT_PROT_PARSING_STATE_FINISH)
	{
		eat_len = i;
		roboeffect_prot_reset_parse(parse);
	}
	else
	{
		/*current frame not finished*/
		eat_len = parse->start_pos;
		parse->start_pos = 0;
		parse->cur_pos = len - eat_len;
	}

	if(len - eat_len)
		mv_mwrite(&buf[eat_len], 1, len - eat_len, parse->buf);

	return;
}

//length cannot lager than 512
void roboeffect_prot_parse_package_outside(uint8_t *data, uint16_t length)
{
	int len;
	roboeffect_prot_parse *parse = &g_roboeffect_prot_parse;

	len = mv_msize(parse->buf);
	if(len > ROBOEFFECT_SMALL_BUFFER_SIZE)
		len = ROBOEFFECT_SMALL_BUFFER_SIZE;

	if(len + length > ROBOEFFECT_SMALL_BUFFER_SIZE - 1)
	{
		ROBOEFFECT_PROT_DEBUG("bufferInfo rest too much.\n");
		mv_mopen(parse->buf, uart_parse_buffer, ROBOEFFECT_SMALL_BUFFER_SIZE, NULL);
		memcpy(prot_tmp_buf, data, length);
		len = length;
	}
	else
	{
		mv_mread(prot_tmp_buf, 1, len, parse->buf);
		memcpy(prot_tmp_buf + len, data, length);
		len += length;
	}
	while(len)
	{
		__roboeffect_prot_parse_package(parse, prot_tmp_buf, len);

		//if not in idle state, break to wait for incomplete data.
		if(parse->state != ROBOEFFECT_PROT_PARSING_STATE_NONE) break;

		len = mv_msize(parse->buf);
		if(len > ROBOEFFECT_SMALL_BUFFER_SIZE)
			len = ROBOEFFECT_SMALL_BUFFER_SIZE;
		mv_mread(prot_tmp_buf, 1, len, parse->buf);
	}
}

void roboeffect_prot_parse_big_block(uint8_t *data, uint16_t length)
{
	uint8_t *read_offset = data;
	int32_t rest_size = length;

	while(rest_size)
	{
		int32_t llen = (rest_size>ROBOEFFECT_PARSE_MAX_SIZE)?(ROBOEFFECT_PARSE_MAX_SIZE):(rest_size);
		roboeffect_prot_parse_package_outside(read_offset, llen);

		rest_size -= llen;
		read_offset += llen;
	}
}

MemHandle bufferInfo;
void roboeffect_prot_init(void)
{
	roboeffect_prot_reset_parse(&g_roboeffect_prot_parse);
	memset(&bufferInfo, 0x00, sizeof(MemHandle));

	mv_mopen(&bufferInfo, uart_parse_buffer, ROBOEFFECT_SMALL_BUFFER_SIZE, NULL);
	g_roboeffect_prot_parse.buf = &bufferInfo;
}

#endif //defined(CFG_FUNC_ROBOEFFECT_EN)
