

// Sample : Using messageBuffer APIs using EEP as external Buffer instead of RAM

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "message_buffer.h"
#include "stdbool.h"
#include "utilities.h"
#include "eeprom-board.h"
#include "string.h"


#
#define MSG_MAX_LEN		64

#define EEP_BUFFER_HEAD_INDEX_ADR 			1024
#define EEP_BUFFER_TAIL_INDEX_ADR			EEP_BUFFER_HEAD_INDEX_ADR + 4
#define EEP_BUFFER_START_ADR				EEP_BUFFER_TAIL_INDEX_ADR + 4

#define MSG_FIFO_LEN 						1024 


MessageBufferHandle_t  eepMsgHandle;

void __eepBufEepRead(void* buffer ,size_t index ,size_t sz){
	size_t adr = EEP_BUFFER_START_ADR + index;

	EepromMcuReadBuffer(adr,buffer,sz);

	
}

void __eepBufEepWrite(void* buffer ,size_t index ,size_t sz){
	 
	 size_t adr = EEP_BUFFER_START_ADR + index;
	 EepromMcuWriteBuffer( adr, buffer, sz );
}

void __MsgInitEep(void){
	size_t value = 0x0000 ;

	EepromMcuWriteBuffer(EEP_BUFFER_HEAD_INDEX_ADR ,(uint8_t *)&value, sizeof(value));
	EepromMcuWriteBuffer(EEP_BUFFER_TAIL_INDEX_ADR,(uint8_t *)&value,sizeof(value));
}

void MsgInit(void){
	  
	uint32_t head = 0 ;
	uint32_t tail  = 0 ;

	EepromMcuReadBuffer(EEP_BUFFER_HEAD_INDEX_ADR,(uint8_t *)&head,sizeof(uint32_t));
	EepromMcuReadBuffer(EEP_BUFFER_TAIL_INDEX_ADR,(uint8_t *)&tail,sizeof(uint32_t));
	
	if (( head > MSG_FIFO_LEN ) || (tail > MSG_FIFO_LEN ))
	{
		__MsgInitEep();
	}
	
	eepMsgHandle = xMessageWithExtBufferCreate(MSG_FIFO_LEN,__eepBufEepRead,__eepBufEepWrite,head,tail);
}




 void vApplicationSetStreamBufferContext(void *pxStreamBuffer, size_t head , size_t tail){
	 if ( pxStreamBuffer == loraMsgHandle ){
		 EepromMcuWriteBuffer(EEP_BUFFER_HEAD_INDEX_ADR,(uint8_t *)head,sizeof(size_t));
		 EepromMcuWriteBuffer(EEP_BUFFER_TAIL_INDEX_ADR,(uint8_t *)tail,sizeof(size_t));
	 }
}


uint8_t readMsg(uint8_t * Msgbuffer, uint8_t modePeek){
	uint8_t ret = 0 ;

	
	if (xMessageBufferIsEmpty(loraMsgHandle) != pdTRUE ){
		if (modePeek == pdTRUE )	
		ret = xStreamBufferPeek( eepMsgHandle,
                              Msgbuffer,
                              MSG_MAX_LEN,
                              500 );
		else
			ret = xStreamBufferReceive(eepMsgHandle,
                              Msgbuffer,
                              MSG_MAX_LEN,
                              500 );
	}
	return ret;
} 

uint8_t addMsg ( uint8_t *buffer , uint8_t msgSize  )
{
	
		uint8_t msgbuffer[MSG_MAX_LEN];
		size_t  ret = 0;
		
	// Make sure there is a place in our fifo
	  ret = xMessageBufferSpaceAvailable(loraMsgHandle);
	
    while ( ret  <  msgSize + 4 )  {	
			ret = xMessageBufferReceive( eepMsgHandle,
                              msgbuffer,
                              MSG_MAX_LEN,
                              500 );
			ret = xMessageBufferSpaceAvailable(eepMsgHandle);
	};
	// Here we are sur that there is a place for our message
			do{
			 ret = xMessageBufferSend( eepMsgHandle,
                           buffer,
                           msgSize,
                           500 );
			}while ( ret == 0 );

	
	return ret;
}
