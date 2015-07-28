/*
**
** Thread
**
** Richard Buckley July 28, 2015
**
*/

#include <time>
#include <pthread>
#include <stdint>

namespace ca {
	namespace pdp8 {
		class Thread{
		public:
			Thread();
			virtual ~Thread();
			
			int start();
			
			virtual int run() = 0;
			
		protected:
		};
	} // pdp8
} // ca