/*******************************************************************************
* Copyright (c) 18 dec. 2014
* Aydin Homay
* DEEC/FEUP
* a.e.homay@gmail.com
*******************************************************************************/

#include "omreplicationlayer.h"
#include "fbdkasn1layer.h"
#include "timerha.h"
#include "../../arch/devlog.h"
#include "commfb.h"
#include "../../core/datatypes/forte_dint.h"
//#include "../datatypes/forte_date_and_time.h"
//#include <stdio.h>

using namespace forte::com_infra;

COMReplicationlayer::COMReplicationlayer(CComLayer* pa_poUpperLayer, CCommFB * pa_poComFB) :
CComLayer(pa_poUpperLayer, pa_poComFB), mStatSerBuf(0), mStatSerBufSize(0), mDeserBuf(0), mDeserBufSize(0), 
mDeserBufPos(0), mDIPos(0), mDOPos(0), m_unBufFillSize(0), m_eInterruptResp(e_Nothing)
{
	
}

COMReplicationlayer::~COMReplicationlayer()
{
}

EComResponse COMReplicationlayer::openConnection(char *pa_Replication){

	Offset = CIEC_TIME(pa_Replication);
	m_eConnectionState = e_Connected;
	return e_InitOk;

}


void COMReplicationlayer::closeConnection()
{
	if (0 != m_poBottomLayer){
		m_poBottomLayer->closeConnection();
	}
	m_eConnectionState = e_Disconnected;
}

EComResponse COMReplicationlayer::sendData(void *pa_pvData, unsigned int pa_unSize)
{
	EComResponse eRetVal = e_ProcessDataNoSocket;
	if (m_poBottomLayer != 0)
	{
		int repDataSize = pa_unSize + 9;//Date_And_Time need 9 bytes for serialisation
		TForteByte* recvData = new TForteByte[repDataSize];  		
		
		CIEC_DATE_AND_TIME* currDateTime = new CIEC_DATE_AND_TIME;
		currDateTime->setCurrentTime();
		UInt64DateTime = currDateTime->operator TForteUInt64();
		//Adding Offcet
		CIEC_DATE_AND_TIME *validDateTime = new CIEC_DATE_AND_TIME(UInt64DateTime + Offset.getInMicroSeconds());
		
		int nBuf = CFBDKASN1ComLayer::serializeDataPoint(recvData, repDataSize, *validDateTime);
		if (-1 != nBuf)
		{
			memcpy(&(recvData[nBuf]), pa_pvData, pa_unSize);
			eRetVal = m_poBottomLayer->sendData(recvData, repDataSize);
		}
		else
		{
			eRetVal = e_ProcessDataDataTypeError;
			DEVLOG_ERROR("O2MReplication:: serializeData failed\n");
		}
	}
	return eRetVal;
}

EComResponse COMReplicationlayer::recvData(const void *pa_pvData, unsigned int pa_unSize)
{
	CIEC_DATE_AND_TIME validDateTime;
	CIEC_DATE_AND_TIME *currDateTime = new CIEC_DATE_AND_TIME;
	EComResponse eRetVal = e_Nothing;
	TForteByte *recvData = (TForteByte *)pa_pvData;

	if (m_poFb)
	{
		int nBuf = CFBDKASN1ComLayer::deserializeDataPoint(static_cast<const TForteByte*>(recvData), pa_unSize, validDateTime);
		if (0 <= nBuf)
		{
			//we succesfully got the data for the recieved Date_And_Time
			currDateTime->setCurrentTime();

			if (*currDateTime < validDateTime)
			{
				//Crate Pending Packet for the replication layer queue
				COMData* omData = new COMData;
				omData->pvData = (TForteByte*)recvData;
				omData->unSize = pa_unSize;
				omData->Offset = new CIEC_TIME(Offset);
				omData->validDateTime = validDateTime.operator TForteUInt64();
				memcpy(omData->pvData, recvData, pa_unSize);//Create new copy of data packet

				STimedFBListEntry *sTimeFBListEntry = new STimedFBListEntry();
				sTimeFBListEntry->m_eType = e_SingleShot;
				sTimeFBListEntry->m_poTimedFB = m_poFb;
				CTimerHandler::sm_poFORTETimer->registerTimedFB(sTimeFBListEntry, *omData->Offset);//we successfully received data and registered FB into TimedFB list
				FORTE_TRACE("\nO2MReplication: Function Block (%s) registered in CTimeHandler\n", this->m_poFb->getInstanceName());
				m_poFb->interruptCommFB(this);//we successfully Called CommFB intrrupt for registered FB
				FORTE_TRACE("O2MReplication: Intrrupt for Function Block (%s) registered successfully\n", this->m_poFb->getInstanceName());
				repQueue.push(omData);//we successfully inserted data inside of repQueue
				FORTE_TRACE("O2MReplication: Function Block (%s) inserted in replication queue\n", this->m_poFb->getInstanceName());

				eRetVal = e_InitOk;
				m_unBufFillSize += eRetVal;
				m_eInterruptResp = e_ProcessDataOk;
			}
			else
				eRetVal = m_poTopLayer->recvData(static_cast<const TForteByte*>(recvData) + nBuf, pa_unSize - nBuf);
		}
		else
			eRetVal = e_ProcessDataRecvFaild;
	}
	return eRetVal;
}

EComResponse COMReplicationlayer::processInterrupt()
{
	COMData *omData = new COMData();
	unsigned int nBuf = 9; //The length of DateTime
	if (e_ProcessDataOk == m_eInterruptResp){
		switch (m_eConnectionState){
		case e_Connected:
			if ((0 < m_unBufFillSize) && 0 != m_poTopLayer)
			{
				//Fetch the first packet and send it to button layer
				omData = repQueue.front();
				repQueue.pop();
				FORTE_TRACE("O2MReplication: Function Block (%s) processed intrrupt\n", this->m_poFb->getInstanceName());
				m_eInterruptResp = m_poTopLayer->recvData(static_cast<const TForteByte*>(omData->pvData) + nBuf, omData->unSize - nBuf);
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
