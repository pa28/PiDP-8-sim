/*
**
** Chassis
**
** Richard Buckley July 28, 2015
**
*/

namespace ca {
	namespace pdp8 {
		
		Pannel::Pannel() :
			Device("PNL", "Front Pannel"),
			Thread()
		{
		}
		
		Pannel::~Pannel() {
		}
		
		int Pannel::run() {
			return 0;
		}
		
	} // pdp8
} // ca