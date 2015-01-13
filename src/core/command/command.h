#pragma once

#include "ireciever.h"

namespace forte {
	namespace core {
		namespace commands {

			class Ccommand
			{
			protected:
				CIReciever *mReciever;

			public:
				Ccommand(CIReciever* pa_iReciever);
				~Ccommand();

				virtual int Execute() = 0;
			
			};
		}
	}
}

