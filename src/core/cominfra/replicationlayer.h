#ifndef REPLICATIONLAYER_H_
#define REPLICATIONLAYER_H_

//#include <config.h>
#include "comlayer.h"
#include <sockhand.h>
#include <queue> //std::queue
#include <map> //

namespace forte {

	namespace com_infra {

		struct PendingData
		{
			TForteByte* pa_pvData = NULL;
			unsigned int pa_unSize = 0;
			CIEC_TIME* pa_Offset = NULL;
		};
		class CReplicationlayer :public CComLayer
		{
		public:
			CReplicationlayer(CComLayer* pa_poUpperLayer, CCommFB* pa_poComFB);
			~CReplicationlayer();


			EComResponse sendData(void       *pa_pvData, unsigned int pa_unSize); // top interface, called from top
			EComResponse recvData(const void *pa_pvData, unsigned int pa_unSize);
			EComResponse processInterrupt();

			std::queue<PendingData*> repQueue;
		
		protected:
			

		private:
			

			EComResponse openConnection(char *pa_acLayerParameter);
			void closeConnection();


		
			EComResponse m_eInterruptResp;
			unsigned int m_unBufFillSize;

			TForteByte *mStatSerBuf;
			TForteUInt32 mStatSerBufSize;

			TForteByte *mDeserBuf;
			TForteUInt32 mDeserBufSize;
			unsigned int mDeserBufPos;

			TForteByte mDIPos;
			TForteByte mDOPos;
			unsigned long Offset;
		
		};
	}
}
#endif /* REPLICATIONLAYER_H_ */
