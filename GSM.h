
#ifndef GSM_H
#define GSM_H


typedef enum
{
	GPRS_RESP_BUSY = 0,
	GPRS_RESP_OK,
	GPRS_RESP_ERROR_TRYAGAIN,
	GPRS_RESP_ERROR_NOTSTR,
	GPRS_RESP_ERROR_TIMEOUT,

	GPRS_RESP_ERROR_NO_SMS,
	GPRS_RESP_ERROR_UNREAD_SMS,
	GPRS_RESP_ERROR_READ_SMS,
	GPRS_RESP_ERROR_OTHER_SMS,
	GPRS_RESP_ERROR_NOT_AUTH_SMS,
	//GPRS_RESP_ERROR_AUTH_SMS,
	GPRS_RESP_ERROR_LAST_ITEM,
	GPRS_RESP_ERROR_NOT_FREE,
	GPRS_RESP_ERROR_POSITION_ZERO,
	
} GPRS_RESP_ENUM;

typedef enum
{
	GPRS_MOD_ERROR_NO_RESP = -1,
	GPRS_MOD_ERROR_NO_SIGNAL = -2,
	GPRS_MOD_ERROR_CONFIG = -3,
	GPRS_MOD_ERROR_BAUD_RATE = -4,
	GPRS_MOD_ERROR_SIM_CARD = -5,
	GPRS_MOD_ERROR_CONNECT = -6,
	GPRS_MOD_ERROR_SIGNAL = -7,
	GPRS_MOD_ERROR_START_CONN = -8,
	GPRS_MOD_ERROR_END_CONN = -9,
	
} GPRS_MOD_ERR;	

typedef enum
{
	GPRS_TASK_RESET = 0,
	GPRS_TASK_CHECK,
	GPRS_TASK_PWRKEY,
	GPRS_TASK_INI_SIM,
	GPRS_TASK_INI_GSM,
	GPRS_TASK_INI_GPRS,
	GPRS_TASK_INI_CONEXION,
	GPRS_TASK_CONECTADO,		
	GPRS_TASK_ERROR,
	
} GPRS_TASK_ENUM;

typedef enum
{
	GPRS_PROCESS_SENDCMD = 0,
	GPRS_PROCESS_WAITRSP,
		
} GPRS_PROCESS_ENUM;

typedef enum
{
	GPRS_WAITRSP_LOAD = 0,
	GPRS_WAITRSP_POLL,
		
} GPRS_WAITRSP_ENUM;


typedef enum _lista_at_comandos
{	
	AT_SIGNAL = 1,
	AT_NO_ECHO,
	AT_AUTO_BAUDRATE,
	AT_IPR_9600,
	AT_IPR_115200,
	AT_POWERDOWN,
	AT_OK,
	AT_NO_ERR_CODE,
	AT_SMS_TEXT_MODE,
	AT_STATUS_GPRS,
	AT_CFG_SINGLE_IP,
	AT_TCP_TRANSPARENT_MODE,
	AT_BRING_UP_GPRS,
	AT_GET_LOCAL_IP,
	AT_START_TASK,
	AT_START_CONN_TCP,
	AT_CIP_SHUT,
	AT_SAVE_CFG,
	AT_SMS_STORAGE,
	AT_PHONE_STORAGE,
	AT_SMS_INDICATION,
	AT_ATTACH,
	AT_NUM,
	AT_READ_SMS,
	AT_DELETE_SMS
	
} GPRS_AT_ENUM;

enum _estado_rx {
  RX_NOT_FINISHED = 0,      // not finished yet
  RX_FINISHED,              // finished, some character was received
  RX_FINISHED_STR_RECV,     // finished and expected string received
  RX_FINISHED_STR_NOT_RECV, // finished, but expected string not received
  RX_TMOUT_ERR,             // finished, no character received 
                            // initial communication tmout occurred
};

enum at_resp_enum 
{
  AT_RESP_ERR_NO_RESP = -1,   // nothing received
  AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
  AT_RESP_OK = 1,             // response_string was included in the response

  AT_RESP_LAST_ITEM,
};

void DelayGPRS(unsigned int ms);

void GSM_task_reset(void);
void GSM_process_reset(void);
void GSM_waitrsp_reset(void);

void GSM_recibeCaracteres(char *p);


char * GSM_numtel(void);
char GSM_menuopciones_ready(void);


char GSM_init();
char GSM_configGPRS();
char GSM_checkSignal(int *num);


GPRS_TASK_ENUM RevisarTareasGPRS(void);

#endif  /*GSM_H*/ 
