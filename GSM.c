
#include <xc.h>

#include "GSM.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define _XTAL_FREQ  64000000
#define POWERKEY    LATAbits.LATA0
#define RESET       LATAbits.LATA1


struct _GSM_var
{
	GPRS_TASK_ENUM task_sm;
	GPRS_PROCESS_ENUM process_sm;
	GPRS_WAITRSP_ENUM waitrsp_sm;
	int checksignal_sm;
	int nInt;
	int timeOutResp;
	int miliseg;	
	int i;
	int nivel;
	GPRS_MOD_ERR error;
	
	char flagRxIni;
	char flagRfFin;
    char respuestaAT; 
    char commLineStatus;
	char buffer[256];	
	char numtel[16];
}GSM;

typedef struct
{
	GPRS_AT_ENUM atcmd;
	int delay;
	const char *str;
	char nInt;
	
} GPRS_EnvioCmd;


/*void DelayGPRS(unsigned int ms)
{
	while(ms--)
	{
		__delay_ms(1);
		ClrWdt();
	}
}*/

void sendByteUART(char data)
{
    TX1REG = data;
}

char getByteUART(char * data)
{
    *data = RC1REG;
    return 0;
}


void DisplayGSM_Estado(GPRS_TASK_ENUM task)
{
	char str[4];
	char EstIni[] = "Ini";
	char EstRed[] = "Red";
	char EstCon[] = "Con";
	
	switch (task)
	{
		case GPRS_TASK_INI_CONEXION:
			memcpy(str, EstRed, 6);
			break;
		case GPRS_TASK_CONECTADO:
			memcpy(str, EstCon, 6);
			break;
		case GPRS_TASK_RESET:
		case GPRS_TASK_CHECK:
		case GPRS_TASK_PWRKEY:
		case GPRS_TASK_INI_SIM:
		case GPRS_TASK_INI_GSM:
		case GPRS_TASK_INI_GPRS:		
		default:
			memcpy(str, EstIni, 6);
			break;
	}
	
}


void DisplayGSM_Error(GPRS_MOD_ERR error)
{
	char str[7];
	char ErrRsp[] = "ErrRsp";
	char ErrSim[] = "ErrSim";
	char ErrAnt[] = "ErrAnt";
	char ErrCon[] = "ErrCon";
	char ErrFin[] = "ErrFin";
	char ErrUnk[] = "Err???";
	
	switch (error)
	{
		case GPRS_MOD_ERROR_NO_RESP:
			memcpy(str, ErrRsp, 6);
			break;
		case GPRS_MOD_ERROR_SIM_CARD:
			memcpy(str, ErrSim, 6);
			break;
		case GPRS_MOD_ERROR_SIGNAL:
		case GPRS_MOD_ERROR_NO_SIGNAL:
			memcpy(str, ErrAnt, 6);
			break;
		case GPRS_MOD_ERROR_CONNECT:
		case GPRS_MOD_ERROR_START_CONN:
			memcpy(str, ErrCon, 6);
			break;
		case GPRS_MOD_ERROR_END_CONN:
			memcpy(str, ErrFin, 6);
			break;
		default:
			memcpy(str, ErrUnk, 6);
			return;
	}
	
}


/*
Value	RSSI dBm	Condition
2	-109	Marginal
3	-107	Marginal
4	-105	Marginal
5	-103	Marginal
6	-101	Marginal
7	-99	Marginal
8	-97	Marginal
9	-95	Marginal
10	-93	OK
11	-91	OK
12	-89	OK
13	-87	OK
14	-85	OK
15	-83	Good
16	-81	Good
17	-79	Good
18	-77	Good
19	-75	Good
20	-73	Excellent
21	-71	Excellent
22	-69	Excellent
23	-67	Excellent
24	-65	Excellent
25	-63	Excellent
26	-61	Excellent
27	-59	Excellent
28	-57	Excellent
29	-55	Excellent
30	-53	Excellent
*/
void DisplayGSM_Antena(int nivel)
{
	char antena[3];
	
	if (nivel > 30)
	{
		nivel = 30;
	}
	
	if (nivel >= 20)
	{
		antena[0] = 0x00;
		antena[1] = 0x01;
		antena[2] = 0xFF;
	}
	else if (nivel >= 15)
	{
		antena[0] = 0x00;
		antena[1] = 0x01;
		antena[2] = 0xFE;
	}
	else if (nivel >= 10)
	{
		antena[0] = 0x00;
		antena[1] = 0xFE;
		antena[2] = 0xFE;
	}
	else
	{
		antena[0] = 0x78;
		antena[1] = 0x78;
		antena[2] = 0x78;
	}		

}


/*
Algunos comandos AT:
	AT+CIPSHUT 		desactiva el contexto GPRS PDP(un protocolo de transferencia de datos utilizado en GPRS)
	Devuelve:
		AT+CIPSHUT                                                                                                                                           
		SHUT OK                                                                                                                                              
		  

	AT+CIPCLOSE  	Cierra una conexion TCP/UDP

	AT+CIPSTATUS	Pregunta por el estado de la conexion actual
		Devuelve:
		at+cipstatus                                                                                                                                         
		OK                                                                                                                                                   
		                                                                                                                                                     
		STATE: IP INITIAL                                                                                                                                    
		  

	AT+CNUM			Pregunta por el numero del celular
		Devuelve:
		AT+CNUM                                                                                                                                              
		+CNUM: "","+51945194943",145,7,4                                                                                                                     
		                                                                                                                                                     
		OK 
		
	AT+CLTS = 1 	Habilita actualizar la hora desde la red, se usa el comando AT+CCLK para preguntar x la hora
	Devuelve
		AT+CMGS			Envia un mensaje de texto

		AT+CMGS="945376360"  
		> TEST DE ENVIO DE MENSAJE DE TEXTO                                                                                                                  
		+CMGS: 123                                                                                                                                           
		                                                                                                                                                     
		OK                                                                                                                                                   
		                                                                                                                                                     
		+CMTI: "SM",1   

*/

//===========================================================================================
// funciones Basicas
//===========================================================================================

void GSM_task_reset(void){
	GSM.task_sm	= 0;
}	

void GSM_process_reset(void){
	GSM.process_sm = 0;
}

void GSM_waitrsp_reset(void){
	GSM.waitrsp_sm = GPRS_WAITRSP_LOAD;
}

/*char GSM_getsignal(void)
{
	return GSM.nivel;
}*/

void GSM_checksignal_reset(void)
{
	GSM.checksignal_sm = 0;
}

// Envia una cadena de texto hacia el modulo GSM
void GSM_sendString(const char *s){
	while(*s != 0)
		sendByteUART(*s++);
}

// Envia una cadena de texto desde una array
/*void GSM_sendString2(char *s){
	while(*s != 0)
		sendByteUART(*s++);
}*/

// Funcion de recepcion de datos
void GSM_recibeCaracteres(char *p) 
{
	// Empezamos en uno por que ya ha recibido un byte
    int pc_timeout = 1000;
	int i;
    char error;
    unsigned char *ptr;
    
    // Como ya ha capturado un byte,sumamos uno al puntero
    ptr = (unsigned char *)(p+1);
    i = 1;
    
	// Espera hasta que haya un timeOut
	do{
 	 	error = getByteUART(ptr++);
		i++;					// incrementamos el contador
		*ptr = 0;				// Nos aseguramos que siempre el ultimo byte sea 0
	} while((error != pc_timeout) && i < sizeof(GSM.buffer) - 1);	
	
	// Copiamos toda la cadena recepcionada
	strcpy(GSM.buffer, (char *)p);
	
	// Activamos la bandera que indica que se recepcionó una respuesta del modulo GSM
	GSM.respuestaAT = 1;
}


char GSM_comparaCadenaRecibida(char const *compare_string) 
{
    char *ch;
    char ret_val = 0;

    ch = strstr((const char *)GSM.buffer, compare_string);
    
    if (ch != '\0') 
	{
        ret_val = 1;
    }
    
    return (ret_val);
}


//===========================================================================================
//===========================================================================================


/*char GSM_esperaRespuesta(int timeOutResp)
{
	
	// Espera al timeOut o a que llegue una respuesta AT
	while(GSM.respuestaAT == 0){
		__delay_ms(1);
		if(--timeOutResp == 0)
			break;
	}		
	GSM.commLineStatus = 0;
	// Si ha salido del while por que llegÃ³ una respuesta a un comando AT	
	if(GSM.respuestaAT){
		GSM.respuestaAT = 0;
		return RX_FINISHED;
	}
	
	return RX_TMOUT_ERR;
}*/


char GSM_esperaRespuestaCadena(int timeOutResp, const char *str_esperada)
{
	GPRS_RESP_ENUM res = GPRS_RESP_BUSY;
	
	switch (GSM.waitrsp_sm)
	{
		case GPRS_WAITRSP_LOAD:
			GSM.timeOutResp = timeOutResp;
			GSM.waitrsp_sm = GPRS_WAITRSP_POLL;
			//break;
			
		case GPRS_WAITRSP_POLL:
			__delay_ms(1);
			
			if (GSM.respuestaAT)
			{
				GSM.respuestaAT = 0;
				GSM.commLineStatus = 0;
				//GSM.waitrsp_sm++;	// Incrementa para pasar a un estado de espera				

				if(GSM_comparaCadenaRecibida(str_esperada))
				{
					res = GPRS_RESP_OK;									
				}
				else
				{	
					res = GPRS_RESP_ERROR_NOTSTR;									
				}												
			}
			else if (--GSM.timeOutResp <= 0)
			{
				GSM.commLineStatus = 0;
				//GSM.waitrsp_sm++;				
				res = GPRS_RESP_ERROR_TIMEOUT;
			}
			else
			{
				res = GPRS_RESP_BUSY;
			}
			break;
			
		default:
			;
			break;				
	}

	if (res != GPRS_RESP_BUSY)
	{
		GSM.waitrsp_sm = GPRS_WAITRSP_LOAD;
	}
	
	return res;		
}

/*char GSM_esperaCadenaAsinc(const char *str){
	if(GSM.respuestaAT){
		GSM.respuestaAT = 0;
		if(GSM_comparaCadenaRecibida(str)){
			return RX_FINISHED_STR_RECV;
		}
		else{	
			return RX_FINISHED_STR_NOT_RECV;
		}
	}
	
	return RX_NOT_FINISHED;	
}*/

char GSM_enviaComando(char tipo)
{
	switch(tipo)
	{
		case AT_SIGNAL: 
			GSM_sendString("AT+CSQ\r\n");
			break;
		case AT_AUTO_BAUDRATE:
			GSM_sendString("AT+IPR=0\r\n");	
			break;
		case AT_IPR_9600:
			GSM_sendString("AT+IPR=9600\r\n");	
			break;			
		case AT_IPR_115200:
			GSM_sendString("AT+IPR=115200\r\n");
			break;			
		case AT_POWERDOWN:
			GSM_sendString("AT+CPOWD=1\r\n");
			break;	
		case AT_OK:
			GSM_sendString("AT\r\n");	
			break;
		case AT_STATUS_GPRS:
			GSM_sendString("AT+CGATT?\r\n");
			break;
		case AT_CFG_SINGLE_IP:
			GSM_sendString("AT+CIPMUX=0\r\n");
			break;
		case AT_TCP_TRANSPARENT_MODE:
			GSM_sendString("AT+CIPMODE=1\r\n");
			break;
		case AT_BRING_UP_GPRS:
			GSM_sendString("AT+CIICR\r\n");
			break;	
		case AT_GET_LOCAL_IP:
			GSM_sendString("AT+CIFSR\r\n");	
			break;
		case AT_START_TASK:
			//GSM_sendString("AT+CSTT=\"claro.pe\",\"claro\",\"claro\"\r\n");
			GSM_sendString("AT+CSTT=\"movistar.pe\",\"movistar@datos\",\"movistar\"\r\n");
			//GSM_sendString("AT+CSTT=\"bitel.pe\",\"bitel\",\"bitel\"\r\n");
			//GSM_sendString("AT+CSTT=\"datacard.nextel.com.pe\"\r\n");
			break;	
		case AT_START_CONN_TCP:
			//GSM_sendString("AT+CIPSTART=\"TCP\",\"167.250.205.226\",\"55555\"\r\n");
			GSM_sendString("AT+CIPSTART=\"TCP\",\"www.google.com\",\"80\"\r\n");
			break;
		case AT_CIP_SHUT:
			GSM_sendString("AT+CIPSHUT\r");
			break;
		case AT_NO_ECHO:
			GSM_sendString("ATE0\r\n");
			break;
		case AT_NO_ERR_CODE:
			GSM_sendString("AT+CMEE=0\r\n");
			break;
		case AT_SMS_TEXT_MODE:
			GSM_sendString("AT+CMGF=1\r\n");	// Configura para enviar el SMS modo texto
			break;
		case AT_SMS_INDICATION:
			GSM_sendString("AT+CNMI=2,1\r\n"); 	// almacena codigos no solicitados cuando esta el enlace TA-TE, Envia memoria y posicion de sms 
			break;
		case AT_SMS_STORAGE:
			GSM_sendString("AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
			break;
		case AT_PHONE_STORAGE:
			GSM_sendString("AT+CPBS=\"SM\"\r\n");
			break;
		case AT_SAVE_CFG:
			GSM_sendString("AT&W\r\n");
			break;
		case AT_ATTACH:
			GSM_sendString("AT+CGATT=1\r\n");	
			break;
		case AT_NUM:
			GSM_sendString("AT+CNUM\r\n");
			break;
		case AT_READ_SMS:
			GSM_sendString("+CMGR\r\n");
			break;
		case AT_DELETE_SMS:
			GSM_sendString("AT+CMGD=\r\n");
			break;

	}				
	return 0;
}	

char GSM_enviaCmdAtEsperaResp(char tipo, int tOut, const char *resp, char nInt)
{
	GPRS_RESP_ENUM res;

	switch (GSM.process_sm)
	{
		case GPRS_PROCESS_SENDCMD :
			GSM.nInt = nInt;
			GSM_waitrsp_reset();
			GSM_enviaComando(tipo);
			GSM.process_sm = GPRS_PROCESS_WAITRSP;
			//break;
			
		case GPRS_PROCESS_WAITRSP:
			res = GSM_esperaRespuestaCadena(tOut, resp);			
			if (res != GPRS_RESP_BUSY)
			{
				if (res != GPRS_RESP_OK)
				{	
					if (--GSM.nInt > 0)
					{
						GSM_waitrsp_reset();
						GSM_enviaComando(tipo);
						
						res = GPRS_RESP_BUSY;
					}
				}	
			}
			break;
			
		default:
			;
			break;	
	}
	
	if (res != GPRS_RESP_BUSY)
	{
		GSM.process_sm = 0;
	}
	
	return res;		
}


//===========================================================================================
//===========================================================================================
/*void cerrarConexionGPRS(){

}*/

/*
void GSM_configBaud(void)
{
	unsigned int valBRG;
	long baud;
	char text[64];

	
	switch (n)
	{
		case 0:
			valBRG = 1666;//baudios 2400;
			baud = 2400;
			break;
		case 1:
			valBRG = 832;//baudios 4800;
			baud = 4800;
			break;	
		case 2:
			valBRG = 416;//baudios 9600;
			baud = 9600;
			break;
		case 3:
			valBRG = 207;//baudios 19200;
			baud = 19200;
			break;
		case 4:
			valBRG = 103;//baudios 38400;
			baud = 38400;
			break;
		case 5:
			valBRG = 68;//baudios 57600;
			baud = 57600;
			break;
		case 6:
			valBRG = 34;//baudios 115200;
			baud = 115200;
			break;
		default:
			return 0;		// NO soporta esa velocidad
			break;
	}
	

	valBRG = 34;//baudios 115200;
	baud = 115200;

	// Enviamos el comando AT de configuracion de baudiaje
	sprintf(text, "AT+IPR=%ld\r\n", baud);
	GSM_sendString((const char *)text);//GSM_sendString2(text);
	if(GSM_esperaRespuestaCadena(1000, "OK") == RX_FINISHED_STR_RECV){
		// Si ha respondido OK, cambiamos el baudiaje del microcontrolador
		U2BRG = valBRG;
		// Guarda la configuracion de los baudios
		if(GSM_enviaCmdAtEsperaResp(AT_SAVE_CFG, 1000, "OK", 1) == AT_RESP_OK){
			return 1; 	// La configuracion ha terminado
		}
	}	

	return 0; 
}
*/


void GSM_setBaud(void)
{
	SPBRG1 = 34;
}


void GSM_testBaud(int i)
{
	//GPRS_RESP_ENUM res = GPRS_RESP_ERROR_TIMEOUT;
	unsigned int valBRG;
	//int i;

	switch (i)
	{
		case 0:
			valBRG = 1666;//baudios 2400;
			break;
		case 1:
			valBRG = 832;//baudios 4800;
			break;	
		case 2:
			valBRG = 416;//baudios 9600;
			break;
		case 3:
			valBRG = 207;//baudios 19200;
			break;
		case 4:
			valBRG = 103;//baudios 38400;
			break;
		case 5:
			valBRG = 68;//baudios 57600;
			break;
		case 6:
			valBRG = 34;//baudios 115200;
			break;
		default:
			valBRG = 34;//baudios 115200;
			break;
	}

    SPBRG1 = valBRG;
		
	//res = GSM_enviaCmdAtEsperaResp(AT_OK, 750, "OK", 1);			
	//return res; 	
}


/*char GSM_iniConexionTCP()
{
	// Inicia conexion TCP o UDP
	GSM_enviaCmdAtEsperaResp(AT_START_CONN_TCP, 3000, "CONNECT", 1);

	// Ya ha iniciado la comunicacion TCP
	// Solo hay que esperar que no se cuelgue o se cierre
	return 0;
}*/	

char GSM_checkSignal(int *num)
{
    GPRS_RESP_ENUM res = GPRS_RESP_BUSY;
    const char delim[2] = ":";
    char *ptr;
    
    switch(GSM.checksignal_sm)
    {
    	case 0:
		    if (GSM.commLineStatus == 1)
		    {
		    	//return -1;
		    	return GPRS_RESP_ERROR_NOT_FREE;
		    }	

		    GSM.commLineStatus = 1;
		
		    GSM_enviaComando(AT_SIGNAL);
		    *num = 0;    	
    		GSM_waitrsp_reset();	//Inicia la maquina de estados de espera Respuesta
    		GSM.checksignal_sm++;   		
    		break;   
    		 
    	case 1:
    	    res = GSM_esperaRespuestaCadena(2000, "+CSQ:");
			if (res == GPRS_RESP_OK)
			{
		    	// find out what was received exactly
		        ptr = strstr(GSM.buffer, "+CSQ:");
		        if (ptr != NULL) 
			    {
		            // Deberia pasar la cadena como parametro, pero como ptr tiene la direccion de la cadena
		            ptr = strtok(ptr, delim);
		            ptr = strtok(NULL, delim);
		            *num = atoi(ptr);
		        }
		        
		        //imprime en pantalla el valor
		        DisplayGSM_Antena(GSM.nivel);
   			} 	
   			if (res != GPRS_RESP_BUSY){
   				GSM.commLineStatus = 0;
   			}			
    		break;
    }

	if (res != GPRS_RESP_BUSY)
	{
		GSM.checksignal_sm = 0;
   	} 

    return res;
}


char GSM_DeleteSMS(int position) 
{
	GPRS_RESP_ENUM res;
    //signed char ret_val = -1;
    char text[10];

    if (position == 0) 
    {
    	//return (-3);
    	return GPRS_RESP_ERROR_POSITION_ZERO;
    }
    	
    if (GSM.commLineStatus == 1) 
    {
    	//return (ret_val);
    	return GPRS_RESP_ERROR_NOT_FREE;
    }	
    
    GSM.commLineStatus = 1;
    
    //ret_val = 0; // not deleted yet

    //send "AT+CMGD=XY" - where XY = position
    GSM_enviaComando(AT_DELETE_SMS);	//xfprintf(uart_putc, );
    sprintf(text, "%u\r", (int) position);
    GSM_sendString((const char *)text);//GSM_sendString2(text);

	GSM_waitrsp_reset();

	while ((res = GSM_esperaRespuestaCadena(5000, "OK")) == GPRS_RESP_BUSY);

    /*switch (res) 
	{
        case RX_TMOUT_ERR:
            // response was not received in specific time
            ret_val = -2;
            break;

        case RX_FINISHED_STR_RECV:
            // OK was received => SMS deleted
            ret_val = 1;
            break;

        case RX_FINISHED_STR_NOT_RECV:
            // other response: e.g. ERROR => SMS was not deleted
            ret_val = 0;
            break;
    }*/

    GSM.commLineStatus = 0;

    //return (ret_val);
    return res;
}

/*=============================================================
Funcion que envia SMS:
	number_str:   pointer to the phone number string
	message_str:  pointer to the SMS text string


	Retorna:
        ERROR ret. val:
        ---------------
        -1 - linea de comunicacion GSM no esta libre
        -2 - Gsm no ha respondido, ha ocurrido un timeOut
        -3 - GSM ha respondido con ERROR

        OK ret val:
        -----------
        0 - SMS no fue enviado
        1 - SMS fue enviado


	Ejemplo:
        GSM_SendSMS("00XXXYYYYYYYYY", "SMS text");
===============================================================*/
char GSM_SendSMS(char *number_str, char *message_str) 
{
	GPRS_RESP_ENUM res;
    //char ret_val = -1;
    int i;
	char texto[12] = {0x1a, '\0'};
	
    if (GSM.commLineStatus == 1)
    {
    	//return -1;
    	return GPRS_RESP_ERROR_NOT_FREE;
    }	

    GSM.commLineStatus = 1;

    //ret_val = 0; 
    // Intenta enviar tres veces el SMS en caso que haya algun problema

    for (i = 0; i < 3; i++) 
	{
        // Envia  AT+CMGS="number_str"
        GSM_sendString("AT+CMGS=\"");
        GSM_sendString((const char *)number_str);//GSM_sendString2(number_str);
        GSM_sendString("\"\r");

        //if (RX_FINISHED_STR_RECV == GSM_esperaRespuestaCadena(1000,">")) 
		GSM_waitrsp_reset();
		
        while((res = GSM_esperaRespuestaCadena(1000,">")) == GPRS_RESP_BUSY);
        
        //if (RX_FINISHED_STR_RECV == GSM_esperaRespuestaCadena(1000,">")) 
        if (res == GPRS_RESP_OK) 
	    {
            // send SMS text
            GSM_sendString((const char *)message_str);//GSM_sendString2(message_str);
			GSM_sendString((const char *)texto);//GSM_sendString2(texto);		//xfprintf(uart_putc, "%c", 0x1a);
            //mySerial.flush(); // erase rx circular buffer
            
        	while((res = GSM_esperaRespuestaCadena(7000,"+CMGS")) == GPRS_RESP_BUSY);

            //if (RX_FINISHED_STR_RECV == GSM_esperaRespuestaCadena(7000,"+CMGS"))
        	if (res == GPRS_RESP_OK) 
	        {
                // SMS was send correctly 
                //ret_val = 1;
				// #ifdef DEBUG_PRINT
    			// GSM_DebugPrint("SMS was send correctly \r\n", 0);
				// #endif
                break;
            } 
            else
            {
                continue;
            }    
        } 
        else 
	    {
            // try again
            continue;
        }
    }

    GSM.commLineStatus = 0;
    
    //return (ret_val);
    return res;
}

/*===========================================================================
	Funcion que lee SMS desde una pocision especificada de la SIM

	Posicion:     posicion de SMS <1..20>
	phone_number: Un puntero donde el numero telefonico del SMS recibido sera puesto 
	SMS_text  :   Un puntero donde el teto del SMS sera puesto
	max_SMS_len:  maximo tamaÃ±o del texto del SMS excluyendo tambien el caracter de terminacion de cadena
              
	return: 
        ERROR ret. val:
        ---------------
        -1 - linea de comunicacion no esta libre
        -2 - modulo GSMno ha respondido en un timeout
        -3 - posicion especificada debe ser >0 

        OK ret val:
        -----------
        GETSMS_NO_SMS       - SMS no fue encontrado en posicion especificada 
        GETSMS_UNREAD_SMS   - nuevo SMS fue encontrado en posicion especificada
        GETSMS_READ_SMS     - SMS ya leido fue encontrado en posicion especificada 
        GETSMS_OTHER_SMS    - otro tipo de SMS fue encontrado
=================================================================*/
char GSM_GetSMS(int position, char *phone_number, char *SMS_text, int max_SMS_len) 
{
	GPRS_RESP_ENUM res;
    //signed char ret_val = -1;
    char *p_char;
    char *p_char1;
    int len;
    char txt[64];

    if (position == 0) 
    {
        //return (-3);
		return GPRS_RESP_ERROR_POSITION_ZERO;        
    }    

    if (GSM.commLineStatus == 1)	// Detecta si linea esta ocupada 
    {
        return GPRS_RESP_ERROR_NOT_FREE;
    }    
    
    GSM.commLineStatus = 1;
    
    phone_number[0] = 0; 		// end of string for now
    
    //res = GPRS_RESP_ERROR_NO_SMS; 	// still no SMS

    //send "AT+CMGR=X" - where X = position
    GSM_sendString("AT+CMGR=");
    sprintf(txt, "%u", position);
    GSM_sendString((const char *)txt);//GSM_sendString2(txt);
    GSM_sendString("\r");

    // 5000 msec. for initial comm tmout
    GSM_enviaComando(AT_READ_SMS);
    
    GSM_waitrsp_reset();
    
    while ((res = GSM_esperaRespuestaCadena(7000, "+CMGR")) == GPRS_RESP_BUSY);
       
    switch(res)
	{
        case GPRS_RESP_OK:
            // find out what was received exactly

            //response for new SMS:
            //<CR><LF>+CMGR: "REC UNREAD","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
            //There is SMS text<CR><LF>OK<CR><LF>
            if (GSM_comparaCadenaRecibida("\"REC UNREAD\"")) 
	        {
                // get phone number of received SMS: parse phone number string 
                // +XXXXXXXXXXXX
                // -------------------------------------------------------
                res = GPRS_RESP_ERROR_UNREAD_SMS;
            }
            //response for already read SMS = old SMS:
            //<CR><LF>+CMGR: "REC READ","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
            //There is SMS text<CR><LF>
            else if (GSM_comparaCadenaRecibida("\"REC READ\"")) 
	        {
                // get phone number of received SMS
                // --------------------------------
                res = GPRS_RESP_ERROR_READ_SMS;
            } 
            else 
	        {
                // other type like stored for sending.. 
                res = GPRS_RESP_ERROR_OTHER_SMS;
            }

            // extract phone number string
            // ---------------------------
            p_char = strchr((char *) (GSM.buffer), ',');
            p_char1 = p_char + 2; 	// we are on the first phone number character
            p_char = strchr((char *) (p_char1), '"');
            if (p_char != NULL) 
	        {
                *p_char = 0; 		// end of string
                strcpy(phone_number, (char *) (p_char1));
            }

            // get SMS text and copy this text to the SMS_text buffer
            // ------------------------------------------------------
            p_char = strchr(p_char + 1, 0x0a); // find <LF>
            if (p_char != NULL) 
	        {
                // next character after <LF> is the first SMS character
                p_char++; // now we are on the first SMS character 
                // find <CR> as the end of SMS string
                p_char1 = strchr((char *) (p_char), 0x0d);
                if (p_char1 != NULL) 
	            {
                    // finish the SMS text string 
                    // because string must be finished for right behaviour 
                    // of next strcpy() function
                    *p_char1 = 0;
                }
                // in case there is not finish sequence <CR><LF> because the SMS is
                // too long (more then 130 characters) sms text is finished by the 0x00
                // directly in the WaitResp() routine

                // find out length of the SMS (excluding 0x00 termination character)
                len = strlen(p_char);

                if (len < max_SMS_len) 
	            {
                    // buffer SMS_text has enough place for copying all SMS text
                    // so copy whole SMS text
                    // from the beginning of the text(=p_char position) 
                    // to the end of the string(= p_char1 position)
                    strcpy(SMS_text, (char *) (p_char));
                } 
                else 
	            {
                    // buffer SMS_text doesn't have enough place for copying all SMS text
                    // so cut SMS text to the (max_SMS_len-1)
                    // (max_SMS_len-1) because we need 1 position for the 0x00 as finish 
                    // string character
                    memcpy(SMS_text, (char *) (p_char), (max_SMS_len - 1));
                    SMS_text[max_SMS_len] = 0; // finish string
                }
                
                res = GPRS_RESP_OK;                
            }
            break;

        /*case GPRS_RESP_ERROR_NOTSTR:
            // OK was received => there is NO SMS stored in this position
            if (GSM_comparaCadenaRecibida("OK")) 
	        {
                // there is only response <CR><LF>OK<CR><LF> 
                // => there is NO SMS
                ret_val = GETSMS_NO_SMS;
            } 
            else if (GSM_comparaCadenaRecibida("ERROR")) 
           	{
                // error should not be here but for sure
                ret_val = GETSMS_NO_SMS;
            }
            break;*/
            
        default:
            break;
    }

    GSM.commLineStatus = 0;
    
    return res;
}


//=====================================================
//Solicitar saldo mediante SMS
// MOVISTAR: Enviando la letra S al 550
// CLARO:	 Enviando la palabra Saldo al 777
// BITEL:	 Enviando la palabra SALDO al 164
// ENTEL:    No se puede consultar mediante SMS

enum _tipo_operador{
	OP_MOVISTAR = 0,
	OP_CLARO,
	OP_BITEL,
	OP_ENTEL
};


// Funcion que consulta el saldo
// 0: Ha recibido correctamente la respuesta
// -1: NO hay posicion de SMS
// -2: NO hay respuesta correcta
// -3: maximo tiempo de espera
signed char GSM_consultaSaldo(char operador, char *s){
	signed int res;
	int posSms;
	char *ptr;
	char numTelf[10];
	char smsText[160]; 

	switch(operador){
		case OP_MOVISTAR:
			GSM_SendSMS("550", "S");
			break;
		case OP_CLARO:
			GSM_SendSMS("777","Saldo");
			break;
		case OP_BITEL:
			GSM_SendSMS("164","SALDO");
			break; 
	}

	//Espera durante 2 segundos
	res = GSM_esperaRespuestaCadena(5000, "+CMTI");
	if(res == RX_FINISHED_STR_RECV){
		ptr = strchr((char *)GSM.buffer, ',');
		if(ptr != NULL){
			posSms = atoi(ptr+1);		// convierte en entero la cadena
			GSM_GetSMS(posSms, numTelf, smsText, sizeof(smsText)/sizeof(char));
			s = smsText;
			return 0;
		}
		else
			return -1;
	}
	else if(res == RX_FINISHED_STR_NOT_RECV){
		return -2;
	}
	else{
		return -3;
	}

	return 1;
}
	
/*signed char GSM_consultaNumero(char *s){
	char res;
	char *ptr;

	res = GSM_enviaCmdAtEsperaResp(AT_NUM, 2000, "+CNUM", 1);

	if(res == AT_RESP_OK)
	{
		ptr = strchr((char *)GSM.buffer, ':');
		if(ptr != NULL){
			strcpy(s, ptr+1);
			return 0;
		}
		else
			return -1;
	}
	else
	{
		return -2;
	}
		
	
}
*/

void GSM_get_numtel(void)
{		
	int len;
	char *start_ptr;
	char *end_ptr;	
	char *ptr;

	ptr = strchr((char *)GSM.buffer, ':');
	
	if(ptr != NULL)
	{	
		while(!(*ptr >= '0' && *ptr <= '9'))
		{
			ptr++;
		}
		
		start_ptr = ptr;
		
		while((*ptr >= '0' && *ptr <= '9'))
		{
			ptr++;
		}
		
		end_ptr = ptr;
		
		len = (end_ptr - start_ptr);
		
		if (len > sizeof(GSM.numtel))
		{
			len = sizeof(GSM.numtel);
		}
		
		memcpy(GSM.numtel, start_ptr, len);
	}	
	else
	{
		memset(GSM.numtel, 0, sizeof(GSM.numtel));
	}	
}

char * GSM_numtel(void)
{
	return GSM.numtel;
}

char GSM_menuopciones_ready(void)
{
	if ((GSM.task_sm == GPRS_TASK_INI_CONEXION)||(GSM.task_sm == GPRS_TASK_CONECTADO))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//==========================================================================
//		MAQUINA DE ESTADOS DEL EQUIPO
//==========================================================================

#define GSM_cargaParametros(x, a, b, c, d)	{Envio[x].atcmd = a; Envio[x].delay = b; Envio[x].str = c; Envio[x].nInt = d;}

#define GSM_delayIni(x)		{GSM.miliseg = x;}


char GSM_delayTest(){
	__delay_ms(1); 
	if(--GSM.miliseg <= 0){
		return 1;
	}	
	return 0;
}	


void GSM_Init(void)
{
	//task_init_sm = 0;
	GSM_task_reset();
	GSM_process_reset();	
}

/*
 * Funcion RevisarTareasGPRS
 * 
 * Funcion que encapsula las tareas para el correcto funcionamiento
 * del modulo SIM800L: reset, encendido, configuracion, conexion.
 */

GPRS_TASK_ENUM RevisarTareasGPRS(void)
{	
	GPRS_RESP_ENUM res;

	static GPRS_TASK_ENUM last_task = 0xFF;
	static char task_init_sm = 0;
	static GPRS_EnvioCmd Envio[10];
	
	char atOk[] = "OK";
	char atRdy[] = "RDY";
	char atCpms[] = "+CPMS:";
	char atCgatt[] = "+CGATT: 1";
	char atSmsRdy[] = "SMS Ready";	
	char atCnum[] = "+CNUM";
	
    /*
     * Maquina de estado principal de la gestion del modulo SIM800L
     */
	switch(GSM.task_sm)
	{
        /*
         * En este estado se realiza la labor de resetear el microcontrolador,
         * para asegurarnos de que siempre inicie de recién encendido.
         */
		case GPRS_TASK_RESET:
			// Primero realiza un reset al modulo por si ya estaba encendido
			switch(task_init_sm)
			{
				case 0:
                    /*
                     * Activamos el pin de reset por 1000 milisegundos
                     */
					DisplayGSM_Antena(0);				
					RESET = 0;
					GSM_delayIni(1000);
					task_init_sm++;
					break;
				case 1:
                    /*
                     * Esperamos que termine los 1000 milisegundos para desactivar
                     * el pin de reset.
                     */
					if (GSM_delayTest())
					{
						RESET = 1;	
						GSM_process_reset();
						GSM.task_sm = GPRS_TASK_CHECK;
					}
					break;
			}
			break;
			
        /*
         * En este estado se espera recibir la palabra RDY como confirmacion
         * de que el modulo SIM800L ha salido del estado reset.
         */
		case GPRS_TASK_CHECK:
			// Si el modulo responde con la palabra RDY es por que ya esta encendido y es el correcto Baud
			res = GSM_enviaCmdAtEsperaResp(0, 5000, atRdy, 3);
			if (res == GPRS_RESP_OK){
				task_init_sm = 0;
				GSM.task_sm = GPRS_TASK_INI_SIM;	// = GPRS_TASK_INI_CONEXION;				
			}
            // Sino, entonces regresa al estado GPRS_TASK_PWRKEY
			else if(res != GPRS_RESP_BUSY){			// Si el modulo respondió con timeout o no cadena, tal vez estee apagado
				task_init_sm = 0;
				GSM.task_sm = GPRS_TASK_PWRKEY;		
			}
			break;

        /*
         * En este estado se inicia la configuracion basica del modulo SIM800L
         * donde se establece: no eco, modo texto del sms.
         * Tambien se obtiene el numero de telefono del modulo SIM800L
         */
		case GPRS_TASK_INI_GSM:
			switch(task_init_sm)
			{
				case 0:
					// Leemos por lo menos una vez el nivel de senal
					GSM_checksignal_reset();
					task_init_sm++;
					break;
				case 1:
                    // Confirmamos que no tenga procesos pendiente
					if(GSM_checkSignal(&GSM.nivel) != GPRS_RESP_BUSY){
						task_init_sm++;
					}
					break;
				case 2:
                    //Cargamos todas las configuraciones
					GSM_cargaParametros(0, AT_OK, 1000, atOk, 3);
					GSM_cargaParametros(1, AT_NO_ECHO, 1000, atOk, 3);
					GSM_cargaParametros(2, AT_NO_ERR_CODE, 1000, atOk, 3);
					GSM_cargaParametros(3, AT_SMS_TEXT_MODE, 2000, atOk, 3);
					GSM_cargaParametros(4, AT_SMS_INDICATION, 1000, atOk, 3);
					GSM_cargaParametros(5, AT_SMS_STORAGE, 2000, atCpms, 1);
					GSM_cargaParametros(6, AT_PHONE_STORAGE, 2000, atOk, 1);
					GSM_cargaParametros(7, AT_SAVE_CFG, 1000, atOk, 1);							
					GSM_cargaParametros(8, AT_NUM, 2000, atCnum, 1);							
														
					GSM.i = 0;				// Indica el contador para enviar los comandos AT
					task_init_sm++;
					break;
				case 3:
                    //Espera que el modulo SIM800L responda al comando 
					res = GSM_enviaCmdAtEsperaResp(Envio[GSM.i].atcmd, Envio[GSM.i].delay, Envio[GSM.i].str, Envio[GSM.i].nInt);
                    //Determina si la respuesta es OK
					if (res == GPRS_RESP_OK)
					{								
						//if (++GSM.i >= 8)
						if (++GSM.i >= 9)
						{					
							GSM_get_numtel();																														
							task_init_sm = 0;
							GSM.task_sm = GPRS_TASK_INI_GPRS;
						}							
					}
                    //Determina si la respuesta es distinta de BUSY
					else if(res != GPRS_RESP_BUSY)
					{
						if (GSM.i == 8)
						{
							memset(GSM.numtel, 0, sizeof(GSM.numtel));
							
							task_init_sm = 0;
							GSM.task_sm = GPRS_TASK_INI_GPRS;					
						}
						else
						{
							GSM.task_sm = GPRS_TASK_ERROR;
							GSM.error = GPRS_MOD_ERROR_NO_RESP;
						}	
					}
					break;
			}				
			break;									
		
        /*
         * En este estado se inicia la configuracion de red del modulo SIM800L
         * donde se establece: modo del gprs, modo tcp, datos del proveedor.
         * Tambien permite obtener la IP cliente de la conexion establecida.
         */
		case GPRS_TASK_INI_GPRS:
			switch(task_init_sm)
			{
				case 0:
                    //Cargamos todas las configuraciones
					GSM_cargaParametros(0, AT_STATUS_GPRS, 3000, atCgatt, 1);
					GSM_cargaParametros(1, AT_ATTACH, 5000, atOk, 3);
					GSM_cargaParametros(2, AT_CIP_SHUT, 2000, "SHUT OK", 3);	// Nos aseguramos que se inicie el PDP correctamente
					GSM_cargaParametros(3, AT_CFG_SINGLE_IP, 1000, atOk, 3);
					GSM_cargaParametros(4, AT_TCP_TRANSPARENT_MODE, 1000, atOk, 3);
					GSM_cargaParametros(5, AT_START_TASK, 1000, atOk, 1);
					GSM_cargaParametros(6, AT_BRING_UP_GPRS, 5000, atOk, 2);				
					GSM.i = 0;												
					task_init_sm++;
					break;
				case 1:
                    //Espera que el modulo SIM800L responda al comando 
					res = GSM_enviaCmdAtEsperaResp(Envio[GSM.i].atcmd, Envio[GSM.i].delay, Envio[GSM.i].str, Envio[GSM.i].nInt);
                    //Determina si la respuesta es OK
					if (res == GPRS_RESP_OK)
					{								
						if (Envio[GSM.i].atcmd == AT_STATUS_GPRS){
							++GSM.i;//Para saltarse AT_ATTACH 
						}
						if (++GSM.i >= 7){
							GSM_delayIni(1000);
							task_init_sm = 3;
						}							
					}
                    //Determina si la respuesta es distinta de BUSY
					else if(res != GPRS_RESP_BUSY)
					{
						if (Envio[GSM.i].atcmd == AT_STATUS_GPRS)
						{
							++GSM.i;
							GSM_delayIni(3000);
							task_init_sm = 2;
						}
						else
						{
							task_init_sm = 0;
							GSM.task_sm = GPRS_TASK_RESET;
						}
					}									
					break;
				case 2:
                    //Agrega retardo 
					if(GSM_delayTest()){												
						task_init_sm = 1;					
					}
					break;		
				case 3:
                    //Agrega retardo 
					if(GSM_delayTest()){
						task_init_sm++;							
					}
					break;
				case 4:
                    //Obtiene la ip local de cliente
					res = GSM_enviaCmdAtEsperaResp(AT_GET_LOCAL_IP, 1000, "", 3);					
					if (res == GPRS_RESP_OK)
					{
						task_init_sm = 0;
						GSM.task_sm = GPRS_TASK_INI_CONEXION;
					}
					else if(res != GPRS_RESP_BUSY)
					{
						task_init_sm = 0;
						GSM.task_sm = GPRS_TASK_ERROR;
						GSM.error = GPRS_MOD_ERROR_NO_RESP; //GPRS_MOD_ERROR_CONFIG;

					}									
					break;
				default:
					task_init_sm = 0;
					break;
			}				
			break;
			
        /*
         * En este estado iniciamos la conexion TCP del SIM800L en modo cliente
         * con un determinado servidor. La conexion al servidor requiere de
         * un puerto y una IP, la que puede obtnerse mediante DNS desde un URL.
         * Se queda esperando que el modulo responda con el string "CONECTADO".
         */
		case GPRS_TASK_INI_CONEXION:
			switch(task_init_sm){
				case 0:
                    //Confirma que exsite buena señal de antena
					res = GSM_checkSignal(&(GSM.nivel));
					if(res != GPRS_RESP_BUSY){
						if(GSM.nivel < 5){						
							GSM.task_sm = GPRS_TASK_ERROR;
							GSM.error = GPRS_MOD_ERROR_SIGNAL;		// ERROR: NIVEL SE SEÑAL INSUFICIENTE
						}
						else{
							GSM_process_reset();
							task_init_sm++;
						}
					}
					break;
				case 1:
                    //Envia el comando de conexion TCP con los datos del servidor IP y PUERTO
					res = GSM_enviaCmdAtEsperaResp(AT_START_CONN_TCP, 1000, atOk, 2);
					if(res == GPRS_RESP_OK){
						task_init_sm++;
						GSM_waitrsp_reset();						
					}					
					else if (res != GPRS_RESP_BUSY){
						task_init_sm = 0;
						GSM.task_sm = GPRS_TASK_ERROR;
						GSM.error = GPRS_MOD_ERROR_START_CONN;			// ERROR: NO ES POSIBLE CONECTARSE POR GPRS
					}
					break;	
				case 2:
                    //Espera que el modulo responda CONNECT durante 5000 milisegundos
					res = GSM_esperaRespuestaCadena(5000, "CONNECT");
					if(res ==  GPRS_RESP_OK){
						task_init_sm = 0;
						GSM.task_sm = GPRS_TASK_CONECTADO;	
					}
					else if(res != GPRS_RESP_BUSY){
						task_init_sm = 0;
						GSM.task_sm = GPRS_TASK_ERROR;
						GSM.error = GPRS_MOD_ERROR_START_CONN;			// ERROR: NO ES POSIBLE CONECTARSE POR GPRS
					}
					break;					
			}			
			break;

        /*
         * En este estado mantenemos la conexion TCP de cliente SIM800L con
         * el servidor remoto. Se queda en este estado mientras el modulo
         * no retorne la cadena CLOSED.
         */
		case GPRS_TASK_CONECTADO:
			switch(task_init_sm){
				case 0:
                    //Inicializa variables
					GSM_waitrsp_reset();
					task_init_sm++;
					break;
				case 1:
                    //Estamos esperando que el modulo se desconecte o que el
                    //servidor cierre la conexion TCP
					res = GSM_esperaRespuestaCadena(10000, "CLOSED");
					if(res == GPRS_RESP_OK){
						task_init_sm = 0;
						GSM.task_sm = GPRS_TASK_INI_CONEXION;
					}
					else if(res == GPRS_RESP_ERROR_NOTSTR){		// MENSAJE PUEDE SER +PDP: DEACT
						task_init_sm = 0;
						GSM.task_sm = GPRS_TASK_ERROR;
						GSM.error = GPRS_MOD_ERROR_END_CONN;			// ERROR: NO ES POSIBLE CONECTARSE POR GPRS	
					}
					break;						
			}		
			break;

        /*
         * En este estado se gestiona el error para que se reinicie
         * la maquina de estado
         */
		case GPRS_TASK_ERROR:
		default:
            task_init_sm = 0;
			break;
	}

	if (last_task != GSM.task_sm)
	{
		DisplayGSM_Estado(GSM.task_sm);
		last_task = GSM.task_sm;
	}

	return GSM.task_sm;
}
