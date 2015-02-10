/*!\Many to One replication layer 
*
* @author AH
@version 20150208/AH - First version as basis for Many to One replication layer
*/

#include "moreplicationlayer.h"
#include "fbdkasn1layer.h"
#include "timerha.h"
#include "../../arch/devlog.h"
#include "commfb.h"
#include "../../core/datatypes/forte_dint.h"

using namespace forte::com_infra;

CMOReplicationlayer::CMOReplicationlayer(CComLayer* pa_poUpperLayer, CCommFB * pa_poComFB) :
CComLayer(pa_poUpperLayer, pa_poComFB), mStatSerBuf(0), mStatSerBufSize(0), mDeserBuf(0), mDeserBufSize(0),
mDeserBufPos(0), mDIPos(0), mDOPos(0), m_unBufFillSize(0), m_eInterruptResp(e_Nothing)
{
	CIEC_DATE_AND_TIME* tmpDateTime = new CIEC_DATE_AND_TIME;
	tmpDateTime->setCurrentTime();
	UInt64DateTime = tmpDateTime->operator TForteUInt64();
}


CMOReplicationlayer::~CMOReplicationlayer()
{
}

EComResponse CMOReplicationlayer::openConnection(char *pa_Replication){

	Offset = CIEC_TIME(pa_Replication);
	m_eConnectionState = e_Connected;
	return e_InitOk;
}


void CMOReplicationlayer::closeConnection()
{
	if (0 != m_poBottomLayer){
		m_poBottomLayer->closeConnection();
	}
	m_eConnectionState = e_Disconnected;
}


EComResponse CMOReplicationlayer::sendData(void *pa_pvData, unsigned int pa_unSize)
{
	EComResponse eRetVal = e_ProcessDataNoSocket;
	if (m_poBottomLayer != 0)
	{
		int repDataSize = pa_unSize + 9;//Date_And_Time need 9 bytes for serialisation
		TForteByte* recvData = new TForteByte[repDataSize];
		TForteUInt64 UInt64ValidDateTime = UInt64DateTime + Offset.getInMicroSeconds();
		CIEC_DATE_AND_TIME *validDateTime = new CIEC_DATE_AND_TIME(UInt64ValidDateTime);

		int nBuf = CFBDKASN1ComLayer::serializeDataPoint(recvData, repDataSize, *validDateTime);
		if (-1 != nBuf)
		{
			memcpy(&(recvData[nBuf]), pa_pvData, pa_unSize);
			eRetVal = m_poBottomLayer->sendData(recvData, repDataSize);
		}
		else
		{
			eRetVal = e_ProcessDataDataTypeError;
			DEVLOG_ERROR("M2OReplication:: serializeData failed\n");
		}
	}
	return eRetVal;
}

EComResponse CMOReplicationlayer::recvData(const void *pa_pvData, unsigned int pa_unSize)
{
	CIEC_DATE_AND_TIME validDateTime;
	CIEC_DATE_AND_TIME *currDateTime = new CIEC_DATE_AND_TIME;
	EComResponse eRetVal = e_Nothing;
	TForteByte *recvData = (TForteByte*)pa_pvData;

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
				CMOData* moData = new CMOData;
				moData->pvData = (TForteByte*)recvData;
				moData->unSize = pa_unSize;
				moData->Offset = new CIEC_TIME(Offset);
				moData->validDateTime = (validDateTime.operator TForteUInt64());
				memcpy(moData->pvData, recvData, pa_unSize);//Create new copy of data packet
				
				//TODO: Askin with Professor about registerTimedFB scenario for other data with unregistered intrrupt
				STimedFBListEntry *sTimeFBListEntry = new STimedFBListEntry();
				sTimeFBListEntry->m_eType = e_SingleShot;
				sTimeFBListEntry->m_poTimedFB = m_poFb;
				CTimerHandler::sm_poFORTETimer->registerTimedFB(sTimeFBListEntry, *moData->Offset);//we successfully received data and registered FB into TimedFB list
				FORTE_TRACE("\nM2OReplication: Function Block (%s) registered in CTimeHandler\n", this->m_poFb->getInstanceName());

				if (!ExistInQueue(moData->validDateTime))//NOTE: Don`t register new interrupt if we have already another interrupted  message inside of replication queue
				{
					m_poFb->m_Blocked = true;
					m_poFb->interruptCommFB(this);//we successfully Called CommFB intrrupt for registered FB
					FORTE_TRACE("M2OReplication: Intrrupt for Function Block (%s) registered successfully\n", this->m_poFb->getInstanceName());
				}
				repQueue.insert(std::pair<TForteUInt64,CMOData*>(moData->validDateTime,moData));//we successfully inserted data inside of repQueue
				FORTE_TRACE("M2OReplication: Function Block (%s) inserted in replication queue\n", this->m_poFb->getInstanceName());

				eRetVal = e_InitOk;
				m_unBufFillSize += eRetVal;
				m_eInterruptResp = e_ProcessDataOk;

			}
			else
			{
				if (this->m_poFb->getComServiceType() == e_Subscriber)
					FORTE_TRACE("M2OReplication:None-Voted Function Block is:(%s) \n", this->m_poFb->getInstanceName());
				eRetVal = m_poTopLayer->recvData(static_cast<const TForteByte*>(recvData)+nBuf, pa_unSize - nBuf);

			}
		}
		else
			eRetVal = e_ProcessDataRecvFaild;
	}
	return eRetVal;
}

EComResponse CMOReplicationlayer::processInterrupt()
{
	m_eInterruptResp = e_ProcessDataOk;
	unsigned int nBuf = 9; //The length of DateTime
	if (e_ProcessDataOk == m_eInterruptResp){
		switch (m_eConnectionState){
		case e_Connected:
			if ((0 < m_unBufFillSize) && 0 != m_poTopLayer && !(this->m_poFb->m_Blocked))
			{
				CMOData moData = Voter();//select data from replication queue regarding of voting alghoritm
				FORTE_TRACE("M2OReplication:Voted Function Block (%s) processed intrrupt\n", this->m_poFb->getInstanceName());
				m_eInterruptResp = m_poTopLayer->recvData(static_cast<const TForteByte*>(moData.pvData) + nBuf, moData.unSize - nBuf);
				m_unBufFillSize = 0;
			}
			else
				m_eInterruptResp = e_Blocked;
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

CMOReplicationlayer::CMOData  CMOReplicationlayer::Voter()
{ 
	//TODO: Implement strategic design pattern for this method
	CMOData voted = *repQueue.begin()->second;
	bool result = repQueue.erase(voted.validDateTime);
	return voted;
}

bool CMOReplicationlayer::ExistInQueue(TForteUInt64 pa_validDateTime)
{
	std::multimap<TForteUInt64, CMOData*>::iterator result = repQueue.find(pa_validDateTime);
	return result == repQueue.end()? false:true;
}
