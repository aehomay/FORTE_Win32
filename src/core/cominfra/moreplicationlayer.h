#ifndef MOREPLICATIONLAYER_H_
#define MOREPLICATIONLAYER_H_

#include "comlayer.h"
#include <sockhand.h>
#include <map> 


namespace forte {

	namespace com_infra {



		class CMOReplicationlayer:public CComLayer
		{
			struct CMOData
			{
				TForteByte* pvData = NULL;//Received data serialization byte
				unsigned int unSize = 0;//The size after adding Date & Time
				CIEC_TIME* Offset = NULL;//Offset Value
				TForteUInt64 validDateTime = NULL;
				bool operator()(CMOData first, CMOData second)
				{
					bool result = false;
					if (first.validDateTime == second.validDateTime)
						result = true;
					return result;
				}
			};

			int compare(const CMOData* first, const CMOData* second)
			{
				if (first->validDateTime == second->validDateTime)
					return 0;
				return first->validDateTime > second->validDateTime ? +1 : -1;
			}


			bool compare_a(const CMOReplicationlayer::CMOData& first, const CMOReplicationlayer::CMOData& second)
			{
				return first.validDateTime < second.validDateTime;
			}

			bool compare_d(const CMOReplicationlayer::CMOData& first, const CMOReplicationlayer::CMOData& second)
			{
				return first.validDateTime > second.validDateTime;
			}

		public:
			CMOReplicationlayer(CComLayer* pa_poUpperLayer, CCommFB* pa_poComFB);
			~CMOReplicationlayer();

			EComResponse sendData(void       *pa_pvData, unsigned int pa_unSize); // top interface, called from top
			EComResponse recvData(const void *pa_pvData, unsigned int pa_unSize);
			EComResponse processInterrupt();

			std::multimap<TForteUInt64,CMOData*> repQueue;

		private:

			//TODO: remove the general tm it is temprory
			tm* genTM;
			

			EComResponse openConnection(char *pa_acLayerParameter);
			void closeConnection();

			virtual CMOData Voter();
			virtual bool ExistInQueue(TForteUInt64 pa_votedData);

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
#endif /* MOREPLICATIONLAYER_H_ */
