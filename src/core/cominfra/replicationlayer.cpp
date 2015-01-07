/*******************************************************************************
* Copyright (c) 18 dec. 2014
* Aydin Homay
* DEEC/FEUP
* a.e.homay@gmail.com
*******************************************************************************/

#include "replicationlayer.h"
#include "../../arch/devlog.h"
#include "commfb.h"
#include "../../core/datatypes/forte_dint.h"
//#include "../datatypes/forte_date_and_time.h"
//#include <stdio.h>

using namespace forte::com_infra;

CReplicationlayer::CReplicationlayer(CComLayer* pa_poUpperLayer, CCommFB * pa_poComFB) :
CComLayer(pa_poUpperLayer, pa_poComFB),
m_nSocketID(CIPComSocketHandler::scm_nInvalidSocketDescriptor),
m_nListeningID(CIPComSocketHandler::scm_nInvalidSocketDescriptor),
m_eInterruptResp(e_Nothing),
m_unBufFillSize(0){
}


CReplicationlayer::~CReplicationlayer()
{
}

EComResponse CReplicationlayer::openConnection(char *){
	//We don't need layer specific parameters
	return e_InitOk;
}

void CReplicationlayer::closeConnection(){
	// we don't need to do anything specific on closing when closing the connection
	//so directly close the bottom layer if there
	if (0 != m_poBottomLayer){
		m_poBottomLayer->closeConnection();
	}
}

EComResponse CReplicationlayer::sendData(void   *pa_pvData, unsigned int pa_unSize){
	EComResponse eRetVal;

	CIEC_DATE_AND_TIME *DateAndTime = new CIEC_DATE_AND_TIME;
	DateAndTime->setCurrentTime();
	TForteUInt64 V;
	V = DateAndTime->operator TForteUInt64();
	// += (Offset * 1000); Homay-1/5/2015 Unknow reason
	(*DateAndTime) = V;
	char s[50];
	DateAndTime->toString(s, 50);
	CIEC_STRING cs(s);

	CIEC_ANY* pc = (CIEC_ANY*)malloc((pa_unSize + 2)*sizeof(CIEC_ANY));

	memcpy((void *)&(pc[0]), (void *)pa_pvData, (pa_unSize)*sizeof(CIEC_ANY));
	memcpy((void *)&(pc[pa_unSize]), (void *)(&cs), sizeof(CIEC_ANY));
	memcpy((void *)&(pc[pa_unSize + 1]), (void *)(&cs), sizeof(CIEC_ANY));


	eRetVal = m_poBottomLayer->sendData(static_cast<void*>(pc), pa_unSize + 2);


	return eRetVal;
}

EComResponse CReplicationlayer::recvData(const void *pa_pvData, unsigned int pa_unSize){

	TForteByte *recvData = (TForteByte *)pa_pvData;
	CIEC_STRING* pcs = (CIEC_STRING*)pa_pvData;

	//CIEC_DATE_AND_TIME *DateAndTime = new CIEC_DATE_AND_TIME;
	//DateAndTime->fromString(pcs->getValue());
	//TForteUInt64 val = (DateAndTime->operator TForteUInt64());

	CIEC_DATE_AND_TIME x;
	x.setCurrentTime();
	TForteUInt64 y;
	y = x.operator TForteUInt64();

	m_poTopLayer->recvData((void *)&pcs[0], pa_unSize);
	return e_ProcessDataOk;
}

void CReplicationlayer::handledConnectedDataRecv(){
	// in case of fragmented packets, it can occur that the buffer is full,
	// to avoid calling receiveDataFromTCP with a buffer size of 0 wait until buffer is larger 0
	while ((cg_unIPLayerRecvBufferSize - m_unBufFillSize) <= 0){
#ifdef WIN32
		Sleep(0);
#else
		sleep(0);
#endif
	}
	if (CIPComSocketHandler::scm_nInvalidSocketDescriptor != m_nSocketID){
		// TODO: sync buffer and bufFillSize
		int nRetVal = 0;
		switch (m_poFb->getComServiceType()){
		case e_Server:
		case e_Client:
			nRetVal =
				CIPComSocketHandler::receiveDataFromTCP(m_nSocketID, &m_acRecvBuffer[m_unBufFillSize], cg_unIPLayerRecvBufferSize
				- m_unBufFillSize);
			break;
		case e_Publisher:
			//do nothing as subscribers cannot receive data
			break;
		case e_Subscriber:
			nRetVal =
				CIPComSocketHandler::receiveDataFromUDP(m_nSocketID, &m_acRecvBuffer[m_unBufFillSize], cg_unIPLayerRecvBufferSize
				- m_unBufFillSize);
			break;
		}
		switch (nRetVal){
		case 0:
			DEVLOG_INFO("Connection closed by peer\n");
			m_eInterruptResp = e_InitTerminated;
			closeSocket(&m_nSocketID);
			if (e_Server == m_poFb->getComServiceType()){
				//Move server into listening mode again
				m_eConnectionState = e_Listening;
			}
			break;
		case -1:
			m_eInterruptResp = e_ProcessDataRecvFaild;
			break;
		default:
			//we successfully received data
			m_unBufFillSize += nRetVal;
			m_eInterruptResp = e_ProcessDataOk;
			break;
		}
		m_poFb->interruptCommFB(this);
	}
}

void CReplicationlayer::handleConnectionAttemptInConnected(){
	//accept and immediately close the connection to tell the client that we are not available
	//sofar the best option I've found for handling single connection servers
	CIPComSocketHandler::TSocketDescriptor socketID = CIPComSocketHandler::acceptTCPConnection(m_nListeningID);
	CIPComSocketHandler::closeSocket(socketID);
}

void CReplicationlayer::closeSocket(CIPComSocketHandler::TSocketDescriptor *pa_nSocketID){
	if (CIPComSocketHandler::scm_nInvalidSocketDescriptor != *pa_nSocketID){
		CIPComSocketHandler::getInstance().removeComCallback(*pa_nSocketID);
		CIPComSocketHandler::closeSocket(*pa_nSocketID);
		*pa_nSocketID = CIPComSocketHandler::scm_nInvalidSocketDescriptor;
	}
}



