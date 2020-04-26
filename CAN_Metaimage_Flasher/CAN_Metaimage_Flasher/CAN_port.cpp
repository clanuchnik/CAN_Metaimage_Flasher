#include "stdafx.h"
#include "windows.h"
#include "CAN.h"
#include "PCANBasic.h"

/* Saves the handle of a PCAN hardware */
TPCANHandle m_PcanHandle = PCAN_USBBUS1;

/* nominal BitRate = 1Mbps, DataBitRate = 5Mbps */
//TPCANBitrateFD bitRate = "f_clock_mhz=40, nom_brp=5, nom_tseg1=5, nom_tseg2=2, nom_sjw=1, data_brp=1, data_tseg1=5, data_tseg2=2, data_sjw=1";

/* nominal BitRate = 500kbps, DataBitRate = 500kbps */
//TPCANBitrateFD bitRate = "f_clock_mhz=40, nom_brp=5, nom_tseg1=11, nom_tseg2=4, nom_sjw=1, data_brp=5, data_tseg1=11, data_tseg2=4, data_sjw=1";

/* nominal BitRate = 1Mbps, DataBitRate = 1Mbps */
TPCANBitrateFD bitRate = "f_clock_mhz=40, nom_brp=5, nom_tseg1=5, nom_tseg2=2, nom_sjw=1, data_brp=5, data_tseg1=5, data_tseg2=2, data_sjw=1";

/* nominal BitRate = 1Mbps, DataBitRate = 2Mbps */
//TPCANBitrateFD bitRate = "f_clock_mhz=40, nom_brp=5, nom_tseg1=5, nom_tseg2=2, nom_sjw=1, data_brp=4, data_tseg1=3, data_tseg2=1, data_sjw=1";

/* nominal BitRate = 500kbps, DataBitRate = 1Mbps */
//TPCANBitrateFD bitRate = "f_clock_mhz=40, nom_brp=5, nom_tseg1=11, nom_tseg2=4, nom_sjw=1, data_brp=5, data_tseg1=5, data_tseg2=2, data_sjw=1";

/* nominal BitRate = 500kbps, DataBitRate = 2Mbps */
//TPCANBitrateFD bitRate = "f_clock_mhz=40, nom_brp=5, nom_tseg1=11, nom_tseg2=4, nom_sjw=1, data_brp=4, data_tseg1=3, data_tseg2=1, data_sjw=1";


int port_canfd_initialize()
{
	TPCANStatus stsResult = PCAN_ERROR_OK;

	
	stsResult = CAN_InitializeFD(m_PcanHandle, bitRate);
	if (stsResult != PCAN_ERROR_OK) {
		if (stsResult != PCAN_ERROR_CAUTION)
		{
			printf("error while CanFD Init 0x%x \n", stsResult);
			return -1;
		}
	}
	else
	{
		stsResult = PCAN_ERROR_OK;
	}
	return 0;
}

void port_can_write(t_canfd_msg *can_msg, INT32 datalen)
{
	TPCANMsgFD CANMsg;
	CANMsg.ID = can_msg->ID;
	CANMsg.DLC = can_msg->DLC;
	CANMsg.MSGTYPE = PCAN_MESSAGE_STANDARD;
	CANMsg.MSGTYPE |= PCAN_MESSAGE_FD;
	CANMsg.MSGTYPE |= PCAN_MESSAGE_BRS;
	memcpy(&CANMsg.DATA[0], &(can_msg->DATA[0]), datalen);

	CAN_WriteFD(m_PcanHandle, &CANMsg);
}