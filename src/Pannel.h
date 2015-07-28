/*
**
** Pannel
**
** Richard Buckley July 28, 2015
**
*/

#include <Thread.h>
#include <Device.h>

namespace ca {
	namespace pdp8 {
		class Pannel : public Device, Thread {
		public:
			Pannel();
			virtual ~Pannel();
			
			virtual int run();
			
		protected:
		};
	} // pdp8
} // ca