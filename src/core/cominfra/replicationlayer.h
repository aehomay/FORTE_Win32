#ifndef REPLICATIONLAYER_H_
#define REPLICATIONLAYER_H_

//#include <config.h>
#include "comlayer.h"
#include <sockhand.h>

namespace forte {

	namespace com_infra {

		class CReplicationlayer :public CComLayer
		{
		public:
			CReplicationlayer(CComLayer* pa_poUpperLayer, CCommFB* pa_poComFB);
			~CReplicationlayer();

			
			EComResponse sendData(void       *pa_pvData, unsigned int pa_unSize); // top interface, called from top
			EComResponse recvData(const void *pa_pvData, unsigned int pa_unSize);
		
		protected:

		private:
			static void closeSocket(CIPComSocketHandler::TSocketDescriptor *pa_nSocketID);

			EComResponse openConnection(char *pa_acLayerParameter);
			void closeConnection();
			void handledConnectedDataRecv();
			void handleConnectionAttemptInConnected();

			CIPComSocketHandler::TSocketDescriptor m_nSocketID;
			CIPComSocketHandler::TSocketDescriptor m_nListeningID; //!> to be used by server type connections. there the m_nSocketID will be used for the accepted connection.
			CIPComSocketHandler::TUDPDestAddr m_tDestAddr;
			EComResponse m_eInterruptResp;
			char m_acRecvBuffer[cg_unIPLayerRecvBufferSize];
			unsigned int m_unBufFillSize;
			
			int Offset;
		};
	}
}
#endif /* REPLICATIONLAYER_H_ */
