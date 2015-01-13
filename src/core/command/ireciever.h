#pragma once

namespace forte {
	namespace core {
		namespace commands {

			enum ACTION_LIST
			{

			};

			class CIReciever
			{
			public:
				CIReciever();
				~CIReciever();

				virtual void SetAction(ACTION_LIST action) = 0;
				virtual int GetResult() = 0;
			};
		}
	}
}
