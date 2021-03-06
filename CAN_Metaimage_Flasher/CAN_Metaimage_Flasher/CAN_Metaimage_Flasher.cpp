// CAN_Metaimage_Flasher.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "winbase.h"
#include "tchar.h"
#include "Shlwapi.h"
#include "CAN.h"
#include "PCANBasic.h"

#include <atlstr.h>


int trace_enabled = 0;
TCHAR *metaimage_path_name;

void show_help(){
	printf("|============|\n");
	printf("|HELP section|\n");
	printf("|============|\n");
	printf("USAGE: CAN_Metaimage_Flasher.exe <MetaImage_path_name> <f_clock_mhz=80, nom_brp=10, nom_tseg1=5, nom_tseg2=2, nom_sjw=1, data_brp=2, data_tseg1=5, data_tseg2=2, data_sjw=1>[optional]<trace_enable_flag>\n\n");
	printf("<MetaIMage_path_name>\t=> path and name of the MetaImage\n\n");
	printf("<trace_enable_flag> = 1\t=> Enables trace\n");
	printf("<trace_enable_flag> = 0\t=> Disable trace\n");
	printf("<trace_enable_flag>\t=> Default: trace Disabled\n\n");
	printf("Examples\n");
	printf("CAN_Metaimage_Flasher.exe D:\\Metaimage\\awr_metaimage.bin\n");
	printf("CAN_Metaimage_Flasher.exe D:\\Metaimage\\awr_metaimage.bin 1\n");
}

int check_file_existence(TCHAR *file_path_name) {
	if (0xFFFFFFFF == GetFileAttributes((LPCWSTR)file_path_name)) {
		_tprintf(_T("File: %s doesn't exist\n"), file_path_name);
		return -1;
	}
	else {
		return 0;
	}
}

int _tmain(int argc, TCHAR *argv[]){
	int arg_count = 0;
	errno_t err;
	FILE *metaimage_file_handle;
	long metaimage_file_size = 0;
	int ret_val = 0;

#ifdef DEBUG_ENABLED
	printf("Number of cmd line arg = %d\n", argc);
#endif

	if ((argc < 2) || (argc > 3)) {
		printf("Atleast one argument is mandatory\n");
		printf("Maximum of two arguments are supported\n");
		show_help();
		return -1;
	}
#ifdef DEBUG_ENABLED
	for (arg_count = 0; arg_count < argc; arg_count++) {
		_tprintf(_T("argument %d\t = %s\n"), arg_count, argv[arg_count]);
	}
#endif // DEBUG_ENABLED


	//_tcscpy_s(metaimage_path_name, CA2T(mypath.c_str()));

	metaimage_path_name = _T("C:\\ti\\mmwave_automotive_toolbox_2_9_1\\labs\\lab0012_can_sbl\\pc-app\\xwr18xx_mrr_demo.bin");//Claudio

	//metaimage_path_name = argv[1];


	if (PathFileExists(metaimage_path_name) != 1) {
		ret_val = GetLastError();
		printf("last error = %d\n", ret_val);
		_tprintf(_T("File: %s doesn't exist\n"), metaimage_path_name);
		return -1;
	}

	/* open the firmware file */
	err = _wfopen_s(&metaimage_file_handle, metaimage_path_name, L"rb");

	/* fopen returns 0, the NULL pointer, on failure */
	if (err != 0)
	{
		_tprintf(_T("Could not open file %s \n"), metaimage_path_name);
	}

	/* get the firmware file length */
	fseek(metaimage_file_handle, 0, 2);    /* file pointer at the end of file */
	metaimage_file_size = ftell(metaimage_file_handle);
	rewind(metaimage_file_handle);
#ifdef DEBUG_ENABLED
	printf("Meta Image Size = %d\n", metaimage_file_size);
#endif

	
	if (port_canfd_initialize() < 0) {
		printf("Error in CAN FD initialization\n");
		return -1;
	}
	else {
#ifdef DEBUG_ENABLED
		printf("CAN FD initialized\n");
#endif
	}

	if (argc == 4) {
		if (argv[3] == L"1") {
			trace_enabled = 1;
		}
		else {
			trace_enabled = 0;
		}
		}

	int sent_chunk_count = 0;
	/* get total chunk count multiple of CHUNK_BYTE_SIZE */
	int total_chunk_count = (metaimage_file_size / CHUNK_BYTE_SIZE);
	/* get remaining chunk size */
	int last_chunk_size = (metaimage_file_size % CHUNK_BYTE_SIZE);
#ifdef DEBUG_ENABLED
	printf("total chunk = %d\n", total_chunk_count);
	printf("last_chunk_size = %d\n", last_chunk_size);
#endif
/*****************************************************************************************************
	Sending messages over CANFD to override UART in SBL
	1. Trigger SBL entering programming mode
	2. Trigger SBL start receiving image and programming it into qspi
	Author: Claudio ANUCHNIK
	Date: 26/04/2020
******************************************************************************************************/
// Anachoic Key CAN-FD ID
#define SBL_KEY_ID	0x0CA
#define SLB_PROG_ID 0x0EF
#define KEY_LEN 16
#define PROG_LEN 16

	t_canfd_msg anachoic_override_UART;// Claudio
	anachoic_override_UART.ID = SBL_KEY_ID;
	anachoic_override_UART.DLC = (BYTE)15;

	// sending key:
	byte theKey[] = { 0x55,0xaa,'S','B','L','_','K','E','Y',':',' ',0xCA, 0xEF, 0x00, 0xaa, 0x55 };

	// load key into CAN buffer structure
	memcpy(anachoic_override_UART.DATA, (void*)theKey, 16);   
	// send key right now
	port_can_write(&anachoic_override_UART, KEY_LEN);

	// wait for qflash erase
	Sleep(10000);//To be adjusted...

	// sending trigger:
	// Trigger SBL be ready to receive image over CAN-FD and programming it on qflash
	anachoic_override_UART.ID = SLB_PROG_ID;
	byte theTrigger[] = { 0xaa,0x55,'S','B','L','_','P','R','O','G',':',' ',0xEF, 0xCA, 0x55, 0xaa };

	// load SBL Programming trigger into CAN buffer structure
	memcpy(anachoic_override_UART.DATA, (void*)theTrigger, 16);
	// send Trigger right now
	port_can_write(&anachoic_override_UART, KEY_LEN);

	// wait for SBL to be ready to receive image
	Sleep(1000);//To be adjusted...

/*******************************************************************************************************/
	t_canfd_msg can_msg;

	can_msg.ID = FW_MSG_ID;
	can_msg.DLC = (BYTE)15;
	while (sent_chunk_count != total_chunk_count) {
		/* Attempt to read in bytes from firmware file */
		ret_val = fread_s(&can_msg.DATA[0], CHUNK_BYTE_SIZE, CHUNK_BYTE_SIZE, 1, metaimage_file_handle);
		if (ret_val != 1) {
			printf("error while reading data from File\n");
			ret_val = -1;
			break;
		} else {
			ret_val = 0;
 			port_can_write(&can_msg, CHUNK_BYTE_SIZE);
		}
		/* wait for some moment */
		if (sent_chunk_count % 10 == 0)
		{
			Sleep(25);
		}

		sent_chunk_count++;

		if(sent_chunk_count & 0x10) printf(".");
	}

	if ((ret_val == 0) && (last_chunk_size != 0)) {
		memset(&can_msg.DATA[0], 0, CHUNK_BYTE_SIZE);
		ret_val = fread_s(&can_msg.DATA[0], last_chunk_size, last_chunk_size, 1, metaimage_file_handle);
		if (ret_val != 1) {
			printf("error while reading data from File\n");
			ret_val = -1;
		}
		else {
			ret_val = 0;
			port_can_write(&can_msg, last_chunk_size);
		}
	}

	if (ret_val < 0) {
		return -1;
	}

	/* if metaimage file downloaded successfully then send termination message */
	can_msg.ID = FW_TERMINATE_ID;
	can_msg.DLC = (BYTE)15;
	memset(&can_msg.DATA[0], 0, CHUNK_BYTE_SIZE);
	port_can_write(&can_msg, 15);

	printf("\nBytes sent: 0x%x\n", sent_chunk_count * CHUNK_BYTE_SIZE);
	printf("%d Chunks of %d bytes\n", sent_chunk_count, CHUNK_BYTE_SIZE);
	printf("File Sent...!\n");

	return 0;
}


