#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tuya_ble_port.h"
#include "tuya_ble_type.h"
#include "tuya_ble_log.h"
#include "aes.h"
#include "md5.h"
#include "hmac.h"
//#include "main.h"
#include "tuya_ble_internal_config.h"
#include <stdarg.h>
//#include "portmacro.h"
#include "rtos_api.h"
#include "ble_api.h"
#include "ble_app_func.h"
//#include "bt_app_func.h"
#include "spi_flash.h"
#include "timers.h"
#include "flash_interface.h"
tuya_ble_status_t tuya_ble_gap_advertising_adv_data_update(uint8_t const* p_ad_data, uint8_t ad_len)
{
	printf("###app_set_adv_data ad_len : %d\r\n", ad_len);
	TUYA_BLE_LOG_DEBUG("app_set_adv_data ad_len : %d\r\n", ad_len);
	TUYA_BLE_LOG_HEXDUMP_INFO("AdvertisingData ", p_ad_data, ad_len);
	ble_gs_set_adv_data(&p_ad_data[3],ad_len-3);
	//ble_gs_set_adv_data(p_ad_data,ad_len);
	//app_set_adv_data(p_ad_data,ad_len);
    return TUYA_BLE_SUCCESS;
}


tuya_ble_status_t tuya_ble_gap_advertising_scan_rsp_data_update(uint8_t const *p_sr_data, uint8_t sr_len)
{
	printf("###app_set_scan_rsp_data sr_len ; %d\r\n", sr_len);
	TUYA_BLE_LOG_DEBUG("app_set_scan_rsp_data sr_len ; %d\r\n", sr_len);
	TUYA_BLE_LOG_HEXDUMP_INFO("ScanResponseData", p_sr_data, sr_len);
	ble_gs_set_scan_rsp(p_sr_data,sr_len);
//	app_set_scan_rsp_data(p_sr_data,sr_len);
//    EnableAdvertising();
	printf("###ble_gs_app_start_adv: %d\r\n", ble_gs_app_start_adv()); //ble_gs_app_start_adv();
    return TUYA_BLE_SUCCESS;
}



tuya_ble_status_t tuya_ble_gap_disconnect(void)
{
	TUYA_BLE_LOG_DEBUG("tuya_ble_gap_disconnect ble_app_disconnect\r\n");
//	ble_app_disconnect();
//	EnableAdvertising();
	return TUYA_BLE_SUCCESS;
}


tuya_ble_status_t tuya_ble_gap_addr_get(tuya_ble_gap_addr_t *p_addr)
{
    uint8_t *addr;

    addr = BtDeviceBleAddrGet();
    p_addr->addr_type = TUYA_BLE_ADDRESS_TYPE_PUBLIC;
    memcpy(p_addr->addr,addr,6);
    
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_gap_addr_set(tuya_ble_gap_addr_t *p_addr)
{
	TUYA_APP_LOG_HEXDUMP_INFO("tuya_ble_gap_addr_set", p_addr->addr, 6);
	BtDeviceBleAddrSet(p_addr->addr);
    return TUYA_BLE_SUCCESS;
}


tuya_ble_status_t tuya_ble_gatt_send_data(const uint8_t *p_data,uint16_t len)
{
    uint8_t data_len = len;
    if(data_len>TUYA_BLE_DATA_MTU_MAX)
    {
        data_len = TUYA_BLE_DATA_MTU_MAX;
    }

    if(TuyaBleSendData(p_data,data_len) != 0)
    {
    	return TUYA_BLE_ERR_TIMEOUT;
    }
    return TUYA_BLE_SUCCESS;
}

	
tuya_ble_status_t tuya_ble_common_uart_init(void)
{    
    
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_common_uart_send_data(const uint8_t *p_data,uint16_t len)
{
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_timer_create(void** p_timer_id,uint32_t timeout_value_ms, tuya_ble_timer_mode mode,tuya_ble_timer_handler_t timeout_handler)
{
    *p_timer_id = xTimerCreate(NULL,timeout_value_ms,mode,NULL,timeout_handler);
    return TUYA_BLE_SUCCESS;

}


tuya_ble_status_t tuya_ble_timer_delete(void* timer_id)
{
    xTimerDelete(timer_id,10);
    return TUYA_BLE_SUCCESS;

}

tuya_ble_status_t tuya_ble_timer_start(void* timer_id)
{
	xTimerStart(timer_id,10);
	return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_timer_restart(void* timer_id,uint32_t timeout_value_ms)
{
	xTimerChangePeriod(timer_id,timeout_value_ms,10);
	return TUYA_BLE_SUCCESS;
}


tuya_ble_status_t tuya_ble_timer_stop(void* timer_id)
{
	xTimerStop(timer_id,10);
	return TUYA_BLE_SUCCESS;
}


void tuya_ble_device_delay_ms(uint32_t ms)
{
	osTaskDelay(ms);
}


tuya_ble_status_t tuya_ble_rand_generator(uint8_t* p_buf, uint8_t len)
{
    uint32_t cnt=len/4;
    uint8_t  remain = len%4;
    int32_t temp;
    for(uint32_t i=0; i<cnt; i++)
    {
        temp = rand();
        memcpy(p_buf,(uint8_t *)&temp,4);
        p_buf += 4;
    }
    temp = rand();
    memcpy(p_buf,(uint8_t *)&temp,remain);

    return TUYA_BLE_SUCCESS;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
tuya_ble_status_t tuya_ble_device_reset(void)
{
    NVIC_SystemReset();
    return TUYA_BLE_SUCCESS;
}

void tuya_ble_device_enter_critical(void)
{
	portENTER_CRITICAL();

}

void tuya_ble_device_exit_critical(void)
{
	portEXIT_CRITICAL();

}


tuya_ble_status_t tuya_ble_rtc_get_timestamp(uint32_t *timestamp,int32_t *timezone)
{
    *timestamp = 0;
    *timezone = 0;
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_rtc_set_timestamp(uint32_t timestamp,int32_t timezone)
{

    return TUYA_BLE_SUCCESS;
}


tuya_ble_status_t tuya_ble_nv_init(void)
{    
//    nrf_app_flash_init(true);
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_nv_erase(uint32_t addr,uint32_t size)
{    
    tuya_ble_status_t result = TUYA_BLE_SUCCESS;

    uint32_t erase_pages;
    
    /* make sure the start address is a multiple of FLASH_ERASE_MIN_SIZE */
    if(addr % TUYA_NV_ERASE_MIN_SIZE != 0)
    {
    	TUYA_BLE_LOG_ERROR("the start address is a not multiple of TUYA_NV_ERASE_MIN_SIZE");
        return TUYA_BLE_ERR_INVALID_ADDR;
    }
    
    /* calculate pages */
    erase_pages = size / TUYA_NV_ERASE_MIN_SIZE;
    if (size % TUYA_NV_ERASE_MIN_SIZE != 0) {
        erase_pages++;
    }
    for(int i = 0;i<erase_pages;i++)
    {
    	TUYA_BLE_LOG_INFO("SpiFlashEraseWpr %d \r\n",i+(addr/TUYA_NV_ERASE_MIN_SIZE));
    	SpiFlashEraseWpr(SECTOR_ERASE,i+(addr/TUYA_NV_ERASE_MIN_SIZE),1);
//    	DelayMs(10);
    }
    return result;
}

tuya_ble_status_t tuya_ble_nv_write(uint32_t addr,const uint8_t *p_data, uint32_t size)
{  
	SPI_FLASH_ERR_CODE ret;
	TUYA_BLE_LOG_INFO("SpiFlashWriteWpr addr %08x size %08x\r\n",addr,size);
	ret = SpiFlashWriteWpr(addr,p_data,size,1);
	if(ret!=0)
	{
		TUYA_BLE_LOG_ERROR("SpiFlashWriteWpr ERROR %d \r\n",ret);
		return TUYA_BLE_ERR_UNKNOWN;
	}
    return TUYA_BLE_SUCCESS;
}

tuya_ble_status_t tuya_ble_nv_read(uint32_t addr,uint8_t *p_data, uint32_t size)
{
	uint8_t ret;
	TUYA_BLE_LOG_INFO("SpiFlashReadWpr addr %08x size %08x\r\n",addr,size);
	//ret = SpiFlashReadWpr(addr,p_data,size,10);
	ret = SPiFlashReadData(addr,p_data,size);
	if(ret!=0)
	{
		TUYA_BLE_LOG_ERROR("SpiFlashReadWpr ERROR %d \r\n",ret);
		return TUYA_BLE_ERR_UNKNOWN;
	}
    return TUYA_BLE_SUCCESS;
}


#if TUYA_BLE_USE_OS

bool tuya_ble_os_task_create(void **pp_handle, const char *p_name, void (*p_routine)(void *),void *p_param, uint16_t stack_size, uint16_t priority)
{
	BaseType_t ret;
	ret = xTaskCreate(p_routine,p_name,stack_size,p_param,priority,pp_handle);
	return ret;
}

bool tuya_ble_os_task_delete(void *p_handle)
{
	vTaskDelete(p_handle);
	return TUYA_BLE_SUCCESS;
}

bool tuya_ble_os_task_suspend(void *p_handle)
{
	vTaskSuspend(p_handle);
	return TUYA_BLE_SUCCESS;
}

bool tuya_ble_os_task_resume(void *p_handle)
{
	vTaskResume(p_handle);
	return TUYA_BLE_SUCCESS;
}

bool tuya_ble_os_msg_queue_create( void **pp_handle, uint32_t msg_num, uint32_t msg_size)
{
	if((*pp_handle = xQueueCreate(msg_num,msg_size)) != NULL)
	{
		return 0;
	}else{
		return 1;
	}
}

bool tuya_ble_os_msg_queue_delete(void *p_handle)
{
	vQueueDelete(p_handle);
	return TUYA_BLE_SUCCESS;
}

//bool tuya_ble_os_msg_queue_peek(void *p_handle, uint32_t *p_msg_num)
//{
//	return xQueuePeek(p_handle);
////    return os_msg_queue_peek(p_handle, p_msg_num);
//}

bool tuya_ble_os_msg_queue_send(void *p_handle, void *p_msg, uint32_t wait_ms)
{
	BaseType_t ret;

	if (p_handle == NULL)
	{
		TUYA_BLE_LOG_ERROR("xQueueSend p_handle NULL \r\n");
		return FALSE;
	}
	ret = xQueueSend(p_handle, p_msg, wait_ms);
    return ret;
}

bool tuya_ble_os_msg_queue_recv(void *p_handle, void *p_msg, uint32_t wait_ms)
{
	BaseType_t ret;
	if (p_handle == NULL)
	{
		TUYA_BLE_LOG_ERROR("xQueueReceive p_handle NULL \r\n");
		return FALSE;
	}
	ret = xQueueReceive(p_handle,p_msg,wait_ms);
	return ret;
}

#endif


bool tuya_ble_aes128_ecb_encrypt(uint8_t *key,uint8_t *input,uint16_t input_len,uint8_t *output)
{
    uint16_t length;
    mbedtls_aes_context aes_ctx;
    //
    if(input_len%16)
    {
        return false;
    }

    length = input_len;

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);

    while( length > 0 )
    {
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, input, output );
        input  += 16;
        output += 16;
        length -= 16;
    }

    mbedtls_aes_free(&aes_ctx);

    return true;
}

bool tuya_ble_aes128_ecb_decrypt(uint8_t *key,uint8_t *input,uint16_t input_len,uint8_t *output)
{
    uint16_t length;
    mbedtls_aes_context aes_ctx;
    //
    if(input_len%16)
    {
        return false;
    }

    length = input_len;

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);

    while( length > 0 )
    {
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, input, output );
        input  += 16;
        output += 16;
        length -= 16;
    }

    mbedtls_aes_free(&aes_ctx);

    return true;
}

bool tuya_ble_aes128_cbc_encrypt(uint8_t *key,uint8_t *iv,uint8_t *input,uint16_t input_len,uint8_t *output)
{
    mbedtls_aes_context aes_ctx;
    //
    if(input_len%16)
    {
        return false;
    }

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
    
    mbedtls_aes_crypt_cbc(&aes_ctx,MBEDTLS_AES_ENCRYPT,input_len,iv,input,output);
    //
    mbedtls_aes_free(&aes_ctx);

    return true;
}

bool tuya_ble_aes128_cbc_decrypt(uint8_t *key,uint8_t *iv,uint8_t *input,uint16_t input_len,uint8_t *output)
{
    mbedtls_aes_context aes_ctx;
    //
    if(input_len%16)
    {
        return false;
    }

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);
    
    mbedtls_aes_crypt_cbc(&aes_ctx,MBEDTLS_AES_DECRYPT,input_len,iv,input,output);
    //
    mbedtls_aes_free(&aes_ctx);

    return true;
}


bool tuya_ble_md5_crypt(uint8_t *input,uint16_t input_len,uint8_t *output)
{
    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
    mbedtls_md5_update(&md5_ctx, input, input_len);
    mbedtls_md5_finish(&md5_ctx, output);
    mbedtls_md5_free(&md5_ctx);    
    
    return true;
}

bool tuya_ble_hmac_sha1_crypt(const uint8_t *key, uint32_t key_len, const uint8_t *input, uint32_t input_len, uint8_t *output)
{    
    //hmac_sha1_crypt(key, key_len, input, input_len, output);
	return true;
}

bool tuya_ble_hmac_sha256_crypt(const uint8_t *key, uint32_t key_len, const uint8_t *input, uint32_t input_len, uint8_t *output)
{
   // hmac_sha256_crypt(key, key_len, input, input_len, output);
	return true;
}

void hexdump(char *buf, int len)
{
    int i = 0;
    for(i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if( (i+1) % 16 == 0) {
            printf("\n");
        }
    }
    if(i%16 != 0) {
        printf("\n");
    }
}
