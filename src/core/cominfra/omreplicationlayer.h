#ifndef OMREPLICATIONLAYER_H_
#define OMREPLICATIONLAYER_H_

//#include <config.h>
#include "comlayer.h"
#include <sockhand.h>
#include <queue> //std::queue
#include <map> //

namespace forte {

	namespace com_infra {


		class COMReplicationlayer :public CComLayer
		{
			struct COMData
			{
				TForteByte* pvData = NULL;//Received data serialization byte
				unsigned int unSize = 0;//The size after adding Date & Time
				CIEC_TIME* Offset = NULL;//Offset Value
				TForteUInt64 validDateTime = NULL;
			};

		public:
			COMReplicationlayer(CComLayer* pa_poUpperLayer, CCommFB* pa_poComFB);
			~COMReplicationlayer();


			EComResponse sendData(void       *pa_pvData, unsigned int pa_unSize); // top interface, called from top
			EComResponse recvData(const void *pa_pvData, unsigned int pa_unSize);
			EComResponse processInterrupt();

			std::queue<COMData*> repQueue;
			
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
			CIEC_TIME Offset;
			TForteUInt64 UInt64DateTime;
	

		
		};
	}
}
#endif /* OMREPLICATIONLAYER_H_ */
