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
			TForteByte* pa_pvData = NULL;//Received data serialization byte
			unsigned int pa_unSize = 0;//The size after adding Date & Time
			CIEC_TIME* pa_Offset = NULL;//Offset Value
			TForteUInt64 pa_recvDateTime=0;
			bool operator<(const PendingData& pen)
			{
				return pa_recvDateTime < pen.pa_recvDateTime;
			}
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

			tm genTimeStruct;
			int digit_to_int(char pa_char);
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
			CIEC_TIME Offset;
	

		
		};
	}
}
#endif /* REPLICATIONLAYER_H_ */
