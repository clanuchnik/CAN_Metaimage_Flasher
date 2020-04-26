#pragma once
#include <string>
/* Chunk size of each CAN-FD message */
#define CHUNK_BYTE_SIZE    64
/* Msg-ID for Firmware file over CAN-FD */
#define FW_MSG_ID		   0x29E
/* Msg-ID for termination msg which is being sent after FW is fully sent */
#define FW_TERMINATE_ID    0x19E


/*Debug Flag*/
//#define DEBUG_ENABLED

//std::string mypath = "C:\\";

extern int trace_enabled;
extern TCHAR *metaimage_path_name;

typedef struct canfd_msg_struct
{
	DWORD             ID;       // 11/29-bit message identifier
	BYTE              DLC;      // Data Length Code of the message (0..15)
	BYTE              DATA[64]; // Data of the message (DATA[0]..DATA[63])
} t_canfd_msg;

extern int port_canfd_initialize(void);
extern void port_can_write(t_canfd_msg *can_msg, INT32 datalen);