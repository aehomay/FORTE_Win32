/*******************************************************************************
* Copyright (c) 18 dec. 2014
* Aydin Homay
* DEEC/FEUP
* a.e.homay@gmail.com
*******************************************************************************/

#include "replicationlayer.h"
#include "fbdkasn1layer.h"
#include "timerha.h"
#include "../../arch/devlog.h"
#include "commfb.h"
#include "../../core/datatypes/forte_dint.h"
//#include "../datatypes/forte_date_and_time.h"
//#include <stdio.h>

using namespace forte::com_infra;

CReplicationlayer::CReplicationlayer(CComLayer* pa_poUpperLayer, CCommFB * pa_poComFB) :
CComLayer(pa_poUpperLayer, pa_poComFB), mStatSerBuf(0), mStatSerBufSize(0), mDeserBuf(0), mDeserBufSize(0), 
mDeserBufPos(0), mDIPos(0), mDOPos(0), m_unBufFillSize(0), m_eInterruptResp(e_Nothing)
{

}

CReplicationlayer::~CReplicationlayer()
{
}

EComResponse CReplicationlayer::openConnection(char *pa_Replication){

	//Offset = atoi(pa_Replication) * FORTE_TIME_BASE_UNITS_PER_SECOND;
	//TForteInt32 paOffset = strtol(pa_Replication, NULL, 10);
	//Offset = paOffset * FORTE_TIME_BASE_UNITS_PER_SECOND;
	Offset = CIEC_TIME(pa_Replication);
	m_eConnectionState = e_Connected;
	return e_InitOk;

}

//int CReplicationlayer::digit_to_int(char *pa_char)
//{
//	char str[2];
//	int number = 0;
//	str[0] = pa_char;
//	str[1] = '\0';
//	number = (int)strtol(str, NULL, 10);
//	return number; 
//}

void CReplicationlayer::closeConnection(){
	// we don't need to do anything specific on closing when closing the connection
	//so directly close the bottom layer if there
	if (0 != m_poBottomLayer){
		m_poBottomLayer->closeConnection();
	}
	m_eConnectionState = e_Disconnected;
}

EComResponse CReplicationlayer::sendData(void *pa_pvData, unsigned int pa_unSize)
{
	EComResponse eRetVal = e_ProcessDataNoSocket;

	if (m_poBottomLayer != 0)
	{
		int repDataSize = pa_unSize + 9;//Date_And_Time need 9 bytes for serialisation

		TForteByte* recvData = new TForteByte[repDataSize];  		

		CIEC_DATE_AND_TIME *dateTime = new CIEC_DATE_AND_TIME;
		dateTime->setCurrentTime();
		//Adding the Offset 
		tm *tmDateTime = dateTime->getTimeStruct();
		dateTime->setDateAndTime(*tmDateTime, Offset);
	
		int nBuf = CFBDKASN1ComLayer::serializeDataPoint(recvData, repDataSize, *dateTime);

		if (-1 != nBuf)
		{
			memcpy(&(recvData[nBuf]), pa_pvData, pa_unSize);
			eRetVal = m_poBottomLayer->sendData(recvData, repDataSize);
		}
		else
		{
			eRetVal = e_ProcessDataDataTypeError;
			DEVLOG_ERROR("Replication:: serializeData failed\n");
		}
	}
	return eRetVal;
}

EComResponse CReplicationlayer::recvData(const void *pa_pvData, unsigned int pa_unSize)
{
	CIEC_DATE_AND_TIME recvDateTime;
	CIEC_DATE_AND_TIME *currDateTime = new CIEC_DATE_AND_TIME;
	EComResponse eRetVal = e_Nothing;
	TForteByte *recvData = (TForteByte *)pa_pvData;

	if (m_poFb)
	{
		int nBuf = CFBDKASN1ComLayer::deserializeDataPoint(static_cast<const TForteByte*>(recvData), pa_unSize, recvDateTime);
		if (0 <= nBuf)
		{
			//we succesfully got the data for the recieved Date_And_Time
			currDateTime->setCurrentTime();

			if (*currDateTime < recvDateTime)
			{
				//Crate Pending Packet for the replication layer queue
				PendingData* penData = new PendingData;
				penData->pa_pvData = (TForteByte*)recvData;
				penData->pa_unSize = pa_unSize;
				penData->pa_Offset = new CIEC_TIME(Offset);

 				memcpy(penData->pa_pvData, recvData, pa_unSize);//Create new copy of data packet
				repQueue.push(penData);//Put the packt in queue

				STimedFBListEntry *sTimeFBListEntry = new STimedFBListEntry();
				sTimeFBListEntry->m_eType = e_SingleShot;
				sTimeFBListEntry->m_poTimedFB = m_poFb;

				
				CTimerHandler::sm_poFORTETimer->registerTimedFB(sTimeFBListEntry, *penData->pa_Offset);
				
				eRetVal = e_InitOk;

				//we successfully received data and registered FB into TimedFB list
				m_unBufFillSize += eRetVal;
				m_eInterruptResp = e_ProcessDataOk;
				m_poFb->interruptCommFB(this);//Call CommFB intrrupt
				
			}
			else
				eRetVal = m_poTopLayer->recvData(static_cast<const TForteByte*>(recvData) + nBuf, pa_unSize - nBuf);
		}
		else
			eRetVal = e_ProcessDataRecvFaild;
	}
	return eRetVal;
}

EComResponse CReplicationlayer::processInterrupt()
{
	PendingData *penData = new PendingData();
	unsigned int nBuf = 9; //\The length of DateTime
	if (e_ProcessDataOk == m_eInterruptResp){
		switch (m_eConnectionState){
		case e_Connected:
			if ((0 < m_unBufFillSize) && 0 != m_poTopLayer)
			{
				//Fetch the first packet and send it to button layer
				penData = repQueue.front();
				repQueue.pop();
				
				m_eInterruptResp = m_poTopLayer->recvData(static_cast<const TForteByte*>(penData->pa_pvData) + nBuf, penData->pa_unSize - nBuf);
				m_unBufFillSize = 0;
			}
				break;
		case e_Disconnected:
		case e_Listening:
		case e_ConnectedAndListening:
		default:
			break;
		}
	}
	return m_eInterruptResp;
}