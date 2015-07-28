/*
**
** Chassis
**
** Richard Buckley July 28, 2015
**
*/

namespace ca {
	namespace pdp8 {
		Chassis::Chassis() :
		{
			for (auto &i: deviceList)
				i = NULL;
			
			deviceList[DEV_CPU] = new CPU();
			deviceList[DEV_MEM] = new Memory();
			
			for (auto &i: deviceList) {
				if (i != NULL) {
					i->initialize(this);
				}
			}
		}
		
		Chassis::~Chassis() {
		}
	} // pdp8
} // ca