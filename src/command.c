#include "stm32f0xx.h"                  // Device header

#include "command.h"
#include "serial.h"
#include <stdio.h>
#include <string.h>  // strcmp
#include <ctype.h>   // toupper
#include <stdlib.h>
#include "MS5637.h"
#include "hdc1080.h"

/** Globals */
static char cmdstr_buf [1 + MAX_CMD_LEN];
static char argstr_buf [1 + MAX_CMD_LEN];
char newline [1 + MAX_CMD_LEN];


extern uint8_t  refresh_lcd;
extern uint8_t DeviceAddress(void);

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;



/**
 *
 * Command identifiers	
 */
enum
  {
	CMD_Hdc1080, 				/// Readout from hdc1080
	CMD_MS5637,     		/// Readout from MS5637
	CMD_ID,							/// Identification
		// Add more 
		CMD_LAST
  };

// command table
struct cmd_st
  {
  const char *cmdstr;
  int id;
  };

	
const char helptext[] = 
" ------------- HELP --------------\n\r"
" \n\r ";
	
/**
 *	Command strings - match command with command ID 
 */
const struct cmd_st cmd_tbl [] =
  {
		{	"HDC1080",	CMD_Hdc1080, },
		{ "MS5637",	CMD_MS5637 },
		{ "ID",	CMD_ID },
  };
	
#define CMD_TBL_LEN (sizeof (cmd_tbl) / sizeof (cmd_tbl [0]))
	
/********** Command functions ***********/

void 	cmd_proc_Hdc1080(char *argstr_buf); 			/// @7:hdc1080 th
void 	cmd_proc_MS5637(char *argstr_buf);     		/// @7:MS5637 pt
void 	cmd_proc_ID(char *argstr_buf);								/// Identification


void cmd_unknown(char *argstr_buf);


void cmd_Respond(char *str)
{
	char strout[128];
	
	snprintf(strout, 32, "#%d:%s\n",DeviceAddress(), str);
	uart_puts(strout);
	
}



/*********************************************************************
 * Function:        static unsigned char cmdid_search
 * PreCondition:    -
 * Input:           command string  
 * Output:          command identifier
 * Side Effects:    -
 * Overview:        This function searches the cmd_tbl for a specific 
 *					command and returns the ID associated with that 
 *					command or CID_LAST if there is no matching command.
 * Note:            None
 ********************************************************************/
static int cmdid_search (char *cmdstr) {
	const struct cmd_st *ctp;

	for (ctp = cmd_tbl; ctp < &cmd_tbl [CMD_TBL_LEN]; ctp++) {
		if (strcmp (ctp->cmdstr, cmdstr) == 0) return (ctp->id);
	}

	return (CMD_LAST);
}


/*********************************************************************
 * Function:        char *strupr ( char *src)
 * PreCondition:    -
 * Input:           string  
 * Output:          Uppercase of string
 * Side Effects:    -
 * Overview:        change to uppercase
 * Note:            None
 ********************************************************************/
char *strupr (char *src) {
	char *s;

	for (s = src; *s != '\0'; s++)
		*s = toupper (*s);

	return (src);
}


/*********************************************************************
 * Function:        void cmd_proc (const char *cmd)
 * PreCondition:    -
 * Input:           command line  
 * Output:         	None
 * Side Effects:    Depends on command
 * Overview:        This function processes the cmd command.
 * Note:            The "big case" is here
 ********************************************************************/
void cmd_proc (char *cmd)
{
	char *argsep;
	unsigned int id;
	
	// First part is "@x:" where x is device address
	
	if ((cmd[0]=='@') & (cmd[1] == (DeviceAddress() + '0')) & (cmd[2] == ':'))  // Is this device addressed ?
	{
		/*------------------------------------------------
		First, copy the command and convert it to all
		uppercase.
		------------------------------------------------*/
			strncpy (cmdstr_buf, &cmd[3], sizeof (cmdstr_buf) - 1);
			cmdstr_buf [sizeof (cmdstr_buf) - 1] = '\0';
			strupr (cmdstr_buf);
			//skip empty commands
			if (cmdstr_buf[0] == '\0')
				return;
		/*------------------------------------------------
		Next, find the end of the first thing in the
		buffer.  Since the command ends with a space,
		we'll look for that.  NULL-Terminate the command
		and keep a pointer to the arguments.
		------------------------------------------------*/
			argsep = strchr (cmdstr_buf, ' ');
			
			if (argsep == NULL) {
				argstr_buf [0] = '\0';
			} else {
				strcpy (argstr_buf, argsep + 1);
				*argsep = '\0';
			}

			
		/*------------------------------------------------
		Search for a command ID, then switch on it.  Each
		function invoked here.
		------------------------------------------------*/
			id = cmdid_search (cmdstr_buf);
			
			switch (id)
			{

				case	CMD_Hdc1080: 				/// Hdc1080
					cmd_proc_Hdc1080(argstr_buf);
				break;

				case	CMD_MS5637:     			/// MS5637
					cmd_proc_MS5637(argstr_buf);
				break;

				case	CMD_ID:									/// Identification
					cmd_proc_ID(argstr_buf);
				break;


				case CMD_LAST:
					cmd_unknown(cmdstr_buf);
				break;
			}
			
		
	}		

}


void 	cmd_proc_Hdc1080(char *argstr_buf) 				/// Read from hdc1080
{
  char resp[16];
	uint8_t bat;
	double temp;
	double hum;
	int i;
	
	for (i=0; i<strlen(argstr_buf); i++)
	{
	  hdc1080_measure(&hi2c1, HDC1080_T_RES_14, HDC1080_RH_RES_14, 0, &bat, &temp, &hum);
		switch(argstr_buf[i])
		{
			case 'T' : // temperature
				snprintf(resp, 16, "t=%3.2f", temp);
			break;

			case 'H' : // humidity
			  snprintf(resp, 16, "h=%3.2f", hum);
			break;

			case 'V' : // bat status
			  snprintf(resp, 16, "v=%d", bat);
			break;			
		}
		cmd_Respond(resp);
	}		
}
	
	

void 	cmd_proc_MS5637(char *argstr_buf)						/// Read from MS5637
{
	char resp[16];
  uint8_t i;
	uint16_t Pcal[8];         // calibration constants from MS5637 PROM registers
	uint32_t D1 = 0, D2 = 0;  // raw MS5637 pressure and temperature data
	double Temperature, Pressure; // stores MS5637 pressures sensor pressure and temperature	
	
	for (i=0; i<8; i++)
	{
		MS5637_read_PROM(&hi2c1, i, &Pcal[i]); // c1 to c8 --- 
	}
	
	for (i=0; i<strlen(argstr_buf); i++)
	{
		MS5637_read_ADC_TP(&hi2c1, MS5637_CONVERT_D1_BASE, MS5637_OSR_8192, &D1);  // D1
		MS5637_read_ADC_TP(&hi2c1, MS5637_CONVERT_D2_BASE, MS5637_OSR_8192, &D2);  // D2
		MS5637_Calculate(Pcal, D1, D2, &Temperature, &Pressure);
		switch(argstr_buf[i])
		{
			case 'P' : // pressure
				snprintf(resp, 16, "p=%3.2f", Pressure);
			break;

			case 'T' : // temperature
			  	snprintf(resp, 16, "T=%3.2f", Temperature);
			break;

			case 'C' : // cal[0]
			  snprintf(resp, 16, "c=%04X", Pcal[0]);
			break;		

			case 'D' : // cal[1]
			  snprintf(resp, 16, "d=%04X", Pcal[1]);
			break;		

			case 'E' : // cal[2]
			  snprintf(resp, 16, "e=%04X", Pcal[2]);
			break;		

			case 'F' : // cal[3]
			  snprintf(resp, 16, "f=%04X", Pcal[3]);
			break;		

			case 'G' : // cal[4]
			  snprintf(resp, 16, "g=%04X", Pcal[4]);
			break;		

			case 'H' : // cal[5]
			  snprintf(resp, 16, "h=%04X", Pcal[5]);
			break;		

			case 'I' : // cal[6]
			  snprintf(resp, 16, "i=%04X", Pcal[6]);
			break;		

			case 'J' : // cal[7]
			  snprintf(resp, 16, "j=%04X", Pcal[7]);
			break;		

			case 'x' : // D1
			  snprintf(resp, 16, "x=%08X", D1);
			break;		

			case 'y' : // D2
			  snprintf(resp, 16, "y=%08X", D2);
			break;		
		}
		cmd_Respond(resp);
	}		

}
	

void 	cmd_proc_ID(char *argstr_buf)									/// Identification
{
	cmd_Respond("RHTP V1.0");
}




void cmd_unknown(char *argstr_buf)
{
  cmd_Respond("?");
}

