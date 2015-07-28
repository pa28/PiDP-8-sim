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
		
		void * _threadStart(void *t) {
			Thread *thread = (Thread*)t;
			
			return (void *)(thread->run());
		} 
		
		Thread::Thread() {
		}
		
		Thread::~Thread() {
		}
		
	} // pdp8
} // ca