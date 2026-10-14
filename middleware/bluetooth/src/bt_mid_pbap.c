
#include "type.h"
#include "debug.h"
#include "bt_config.h"
#include "string_convert.h"
#include "bt_pbap_api.h"
#include "bt_manager.h"

#if ((BT_PBAP_SUPPORT)&&(BT_SUPPORT == ENABLE))

//vcard format : v3.0 or v2.1
#define PBAP_VCARD_FORMAT_SELECT		VCARD_FORMAT_30
#define PBAP_GET_LIST_NUMBER_STEP		0xffff//获取全部数据	//50	//每次获取50条数据

//显示PBAP接收的数据
//#define PBAP_INFO_DEBUG

#ifdef PBAP_INFO_DEBUG
uint8_t temp[1024];
char *vcard_begin = "BEGIN:VCARD";//"N;CHARSET=UTF-8:";
char *vcard_end = "END:VCARD";
char *pb_name = "PRINTABLE:";
#endif

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
static void SetPbapState(BT_PBAP_STATE state)
{
	GetBtManager()->pbapState = state;
}

BT_PBAP_STATE GetPbapState(void)
{
	return GetBtManager()->pbapState;
}

/////////////////////////////////////////////////////////////////////////////////
#ifdef PBAP_INFO_DEBUG
int MyStrToHex(char c)   
{   
    if(c>='0'&&c<='9')   
    {   
        return c-0x30;   
    }   
    else if(c>='a'&&c<='f')   
    {   
        return c-0x57;   
    }   
    else if(c>='A'&&c<='F')   
    {   
        return c-0x37;   
    }   
    return 0xfffff;   
} 

static uint16_t PhonePbapCnt=0;
void decode_name(uint8_t *buf)
{
	int i;
	int j =0;
	int k = 0;
	uint8_t tempbuf[64];
	int len = strlen(buf);
	for(i=0;i<len;i++)
	{
		if(buf[i] == '=')
		{
			k++;
			if(k==3)
			{
				break;
			}
		}
	}
	for(j=0;j<len;j++)
	{
		if(buf[j] == 0x0D)
		{
			break;
		}
	}
	memset(tempbuf,0,64);
	memcpy(tempbuf,buf+i,j-i);
	//printf("%d %d %d >>1:%s\n",i,k,j,tempbuf);
	i = 0;
	for(k=0;k<64;k++)
	{
		if(tempbuf[k] == '=')
		{
			tempbuf[i] = tempbuf[k];
			i++;
		}
		else if(MyStrToHex(tempbuf[k]) != 0xfffff)
		{
			tempbuf[i] = tempbuf[k];
			i++;
		}
		else
		{
			
		}
	}
	memset(tempbuf+i,0,64-i);
	k = j;
	j = 0;
	for (i = 0; i <strlen(tempbuf); i++)
	{
		int b = tempbuf[i];
		if (b == '=')
		{
			char u = MyStrToHex(tempbuf[++i]);
			char l = MyStrToHex(tempbuf[++i]);
			tempbuf[j] = (char) ((u << 4) + l);
		}
		else
		{
			tempbuf[j]  = b;
		}
		j++;
	}
	/*memset(tempbuf+j,0,64-j);
	{
		int len = StringConvert(tempbuf,64,tempbuf,strlen(tempbuf),UTF8_TO_GBK);
		memset(tempbuf+len,0,64-len);
		printf("C: %s>>>",tempbuf);
		printf("%s\n",buf+k+2);
	}*/
	
	PhonePbapCnt++;
	printf("%d\n",PhonePbapCnt);
}
#endif


void PbapDataProcess(uint8_t *Buffer,uint32_t Length)
{
#ifdef PBAP_INFO_DEBUG
	int i =0;
	int j = 0;
	int k = 0;
	int start = 0;
	if(temp[0] != 0)
	{
		for(j=0;j<1024;j++)
		{
			if(temp[j] == 0)
			{
				break;
			}
		}
		i = 0;
		for(i=0;i<Length;i++)
		{
			if(memcmp(vcard_end,(uint8_t*)&Buffer[i],sizeof(vcard_end)) == 0)
			{
				memcpy(temp+j,Buffer,i);
				break;
			}
		}
		if(i==Length)
		{
			//printf("error %s %d\n",__FILE__,__LINE__);
		}
		
		{
			int len = StringConvert(temp,1024,temp,strlen(temp),UTF8_TO_GBK);
			memset(temp+len,0,1024-len);
			for(i=0;i<len-sizeof(pb_name);i++)
			{
				if(memcmp(pb_name,temp+i,strlen(pb_name)) == 0)
				{
					i = 1024;
					break;
				}
			}
			if(i==1024)
			{
				//printf(">>>>>>>>%s\n",temp);
				decode_name(temp);
			}
			else
			{
				printf("A: %s\n",temp);
				PhonePbapCnt++;
				printf("%d\n",PhonePbapCnt);
			}
		}
		
		//PhonePbapCnt++;
		//printf("%d\n",PhonePbapCnt);
	}
	memset(temp,0,1024);
	start = 0;
	i = 0;
	j =0;
	k = 0;
	while(1)
	{
		if(memcmp(vcard_begin,(uint8_t*)&Buffer[i],sizeof(vcard_begin)) == 0)
		{
			if(i)
			{
				if(Buffer[i-1] != 'F')
				{
					start = 1;
					memset(temp,0,1024);
				}
			}
		}
		else if(memcmp(vcard_end,(uint8_t*)&Buffer[i],sizeof(vcard_end)) == 0)
		{
			if(start)
			{
				int len = StringConvert(temp,1024,temp,strlen(temp),UTF8_TO_GBK);
				memset(temp+len,0,1024-len);
				for(k=0;k<len-sizeof(pb_name);k++)
				{
					if(memcmp(pb_name,temp+k,strlen(pb_name)) == 0)
					{
						k = 1024;
						break;
					}
				}
				if(k==1024)
				{
					decode_name(temp);
				}
				else
				{
					printf("B: %s\n",temp);
					
					PhonePbapCnt++;
					printf("%d\n",PhonePbapCnt);
				}
				
				//PhonePbapCnt++;
				//printf("%d\n",PhonePbapCnt);
			}
			start = 0;
			j = 0;
			memset(temp,0,1024);
		}
		if(start)
		{
			temp[j] = Buffer[i];
			j++;
		}
		i++;
		if(i>=Length)
		{
			break;
		}
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////
//callback
void BtPbapCallback(BT_PBAP_CALLBACK_EVENT event, BT_PBAP_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		case BT_STACK_EVENT_PBAP_CONNECTING:
			//APP_DBG("rfcomm open ok.pbap start connect\n");
			break;

		case BT_STACK_EVENT_PBAP_CONNECTED:
			APP_DBG("PBAP EVENT:connected\n");
			btManager.pbapGetContinueFlag = 0;
			btManager.pbapGetMaxListCount = 0;
			btManager.pbapListStartOffset = 0;
			SetPbapState(BT_PBAP_STATE_CONNECTED);
		#ifdef PBAP_INFO_DEBUG
			memset(temp,0,1024);
		#endif
			break;

		case BT_STACK_EVENT_PBAP_CONNECT_ERROR:
			APP_DBG("PBAP EVENT:connect error\n");
			break;

		case BT_STACK_EVENT_PBAP_DISCONNECT:
			APP_DBG("PBAP EVENT:disconnect \n");
			btManager.pbapGetContinueFlag = 0;
			SetPbapState(BT_PBAP_STATE_NONE);
			break;

		case BT_STACK_EVENT_PBAP_DISCONNECTING:
			APP_DBG("PBAP EVENT:diconnecting \n");
			break;

		case BT_STACK_EVENT_PBAP_DATA_START:
			PbapDataProcess(param->buffer,param->length);
			APP_DBG("pbap data start size:%d, [%x]\n",param->length,param->buffer[0]);
			break;

		case BT_STACK_EVENT_PBAP_DATA:
			PbapDataProcess(param->buffer,param->length);
			APP_DBG("pbap data size:%d, [%x]\n",param->length,param->buffer[0]);
			break;

		case BT_STACK_EVENT_PBAP_DATA_SINGLE:
			PbapDataProcess(param->buffer,param->length);
			APP_DBG("pbap data single size:%d, [%x]\n",param->length,param->buffer[0]);
			break;

		case BT_STACK_EVENT_PBAP_DATA_END:
			PbapDataProcess(param->buffer,param->length);
			APP_DBG("pbap data end size:%d, [%x]\n",param->length,param->buffer[0]);
			break;

		case BT_STACK_EVENT_PBAP_DATA_END1:
			PbapDataProcess(param->buffer,param->length);
			APP_DBG("pbap data end1 size:%d\n",param->length);
			break;

		case BT_STACK_EVENT_PBAP_NOT_ACCEPTABLE:
			APP_DBG("PBAP: not acceptable\n");
			break;

		case BT_STACK_EVENT_PBAP_NOT_FOUND:
			APP_DBG("PBAP: not found\n");
			break;

 		case BT_STACK_EVENT_PBAP_PACKET_END:
			APP_DBG("PBAP Received ok\n");
 			break;

		case BT_STACK_EVENT_PBAP_CONTINUE_FLAG:
			APP_DBG("continue...  \n");
			btManager.pbapGetContinueFlag = 1;//延时获取PBAP数据,可以使用此标志进行判断,并及时清零
			//可以延时调用此函数,待PBAP数据缓存结束后,再继续获取后续PBAP数据
			PBAP_PullPhoneBook_Continue();
			break;

		case BT_STACK_EVENT_PBAP_APP_PARAM:
			{
				// uint16_t k;
				// APP_DBG("recv app parameters [%d]:  \n", param->length);
				// for(k=0;k<param->length;k++)
				// {
				// 	printf("0x%02x, ", param->buffer[k]);
				// }
				// printf("\n");
				
				//maxlistcount
				btManager.pbapGetMaxListCount = (uint16_t)param->buffer[5]<<8 & 0xff00;
				btManager.pbapGetMaxListCount |= param->buffer[6];

				printf("maxListCount = 0x%x\n", btManager.pbapGetMaxListCount);
			}
			break;
	}
}


/////////////////////////////////////////////////////////////////////////////////
//获取SIM1电话簿信息 
void GetSim1CardPhoneBook(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	APP_DBG("sim1 card:\n");
	//PhonePbapCnt = 0;
	PBAP_PullPhoneBook(SIM1, "pb.vcf", PBAP_VCARD_FORMAT_SELECT, PBAP_GET_LIST_NUMBER_STEP, btManager.pbapListStartOffset);
	btManager.pbapListStartOffset += PBAP_GET_LIST_NUMBER_STEP;
	if(btManager.pbapListStartOffset >= btManager.pbapGetMaxListCount)
		btManager.pbapListStartOffset = 0;
}

//获取SIM1电话簿的数量
void GetSim1CardPhoneBookMaxListCount(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	// PhonePbapCnt = 0;
	btManager.pbapGetMaxListCount = 0;
	btManager.pbapListStartOffset = 0;
	PBAP_PullPhoneBook(SIM1, "pb.vcf", PBAP_VCARD_FORMAT_SELECT, 0, btManager.pbapListStartOffset);
}

//获取SIN2电话簿信息 
void GetSim2CardPhoneBook(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	APP_DBG("sim2 card: %d\n", btManager.pbapListStartOffset);
	//PhonePbapCnt = 0;
	PBAP_PullPhoneBook(SIM2, "pb.vcf", PBAP_VCARD_FORMAT_SELECT, PBAP_GET_LIST_NUMBER_STEP, btManager.pbapListStartOffset);
	btManager.pbapListStartOffset += PBAP_GET_LIST_NUMBER_STEP;
	if(btManager.pbapListStartOffset >= btManager.pbapGetMaxListCount)
		btManager.pbapListStartOffset = 0;
}

//获取SIM2电话簿的数量
void GetSim2CardPhoneBookMaxListCount(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	// PhonePbapCnt = 0;
	btManager.pbapGetMaxListCount = 0;
	btManager.pbapListStartOffset = 0;
	PBAP_PullPhoneBook(SIM2, "pb.vcf", PBAP_VCARD_FORMAT_SELECT, 0, btManager.pbapListStartOffset);
}

//获取手机自身电话簿信息 
void GetMobilePhoneBook(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	APP_DBG("phone book: %d\n", btManager.pbapListStartOffset);
	//PhonePbapCnt = 0;
	PBAP_PullPhoneBook(PHONE, "pb.vcf", PBAP_VCARD_FORMAT_SELECT, PBAP_GET_LIST_NUMBER_STEP, btManager.pbapListStartOffset);
	btManager.pbapListStartOffset += PBAP_GET_LIST_NUMBER_STEP;
	if(btManager.pbapListStartOffset >= btManager.pbapGetMaxListCount)
		btManager.pbapListStartOffset = 0;
}

//获取手机自身电话簿的数量
void GetMobilePhoneBookMaxListCount(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	// PhonePbapCnt = 0;
	btManager.pbapGetMaxListCount = 0;
	btManager.pbapListStartOffset = 0;
	PBAP_PullPhoneBook(PHONE, "pb.vcf", PBAP_VCARD_FORMAT_SELECT, 0, btManager.pbapListStartOffset);
}

//获取呼入电话信息 
void GetIncomingCallBook(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	APP_DBG("Incoming Call:\n");
	PBAP_PullPhoneBook(PHONE, "ich.vcf", PBAP_VCARD_FORMAT_SELECT, PBAP_GET_LIST_NUMBER_STEP, btManager.pbapListStartOffset);
	btManager.pbapListStartOffset += PBAP_GET_LIST_NUMBER_STEP;
	if(btManager.pbapListStartOffset >= btManager.pbapGetMaxListCount)
		btManager.pbapListStartOffset = 0;
}

//获取呼入电话信息数量
void GetIncomingCallBookMaxListCount(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	// PhonePbapCnt = 0;
	btManager.pbapGetMaxListCount = 0;
	btManager.pbapListStartOffset = 0;
	PBAP_PullPhoneBook(PHONE, "ich.vcf", PBAP_VCARD_FORMAT_SELECT, 0, btManager.pbapListStartOffset);
}

//获取呼出电话簿信息 
void GetOutgoingCallBook(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	APP_DBG("Outgoing Call:\n");
	PBAP_PullPhoneBook(PHONE, "och.vcf", PBAP_VCARD_FORMAT_SELECT, PBAP_GET_LIST_NUMBER_STEP, btManager.pbapListStartOffset);
	btManager.pbapListStartOffset += PBAP_GET_LIST_NUMBER_STEP;
	if(btManager.pbapListStartOffset >= btManager.pbapGetMaxListCount)
		btManager.pbapListStartOffset = 0;
}

//获取呼出电话信息数量
void GetOutgoingCallBookMaxListCount(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	// PhonePbapCnt = 0;
	btManager.pbapGetMaxListCount = 0;
	btManager.pbapListStartOffset = 0;
	PBAP_PullPhoneBook(PHONE, "och.vcf", PBAP_VCARD_FORMAT_SELECT, 0, btManager.pbapListStartOffset);
}

//获取未接电话簿信息 
void GetMissedCallBook(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	APP_DBG("Missed Call:\n");
	PBAP_PullPhoneBook(PHONE, "mch.vcf", PBAP_VCARD_FORMAT_SELECT, PBAP_GET_LIST_NUMBER_STEP, btManager.pbapListStartOffset);
	btManager.pbapListStartOffset += PBAP_GET_LIST_NUMBER_STEP;
	if(btManager.pbapListStartOffset >= btManager.pbapGetMaxListCount)
		btManager.pbapListStartOffset = 0;
}

//获取未接电话信息数量
void GetMissedCallBookMaxListCount(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	// PhonePbapCnt = 0;
	btManager.pbapGetMaxListCount = 0;
	btManager.pbapListStartOffset = 0;
	PBAP_PullPhoneBook(PHONE, "mch.vcf", PBAP_VCARD_FORMAT_SELECT, 0, btManager.pbapListStartOffset);
}

void GetCombinedCallBook(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	APP_DBG("Combined Call:\n");
	PBAP_PullPhoneBook(PHONE, "cch.vcf", PBAP_VCARD_FORMAT_SELECT, PBAP_GET_LIST_NUMBER_STEP, btManager.pbapListStartOffset);
	btManager.pbapListStartOffset += PBAP_GET_LIST_NUMBER_STEP;
	if(btManager.pbapListStartOffset >= btManager.pbapGetMaxListCount)
		btManager.pbapListStartOffset = 0;
}

void GetCombinedCallBookMaxListCount(void)
{
	if(GetPbapState() != BT_PBAP_STATE_CONNECTED)
		return;
	
	// PhonePbapCnt = 0;
	btManager.pbapGetMaxListCount = 0;
	btManager.pbapListStartOffset = 0;
	PBAP_PullPhoneBook(PHONE, "cch.vcf", PBAP_VCARD_FORMAT_SELECT, 0, btManager.pbapListStartOffset);
}

#endif

