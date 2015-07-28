/*
**
** Device
**
** Richard Buckley July 28, 2015
**
*/

namespace ca {
	namespace pdp8 {
		
		Device::Device(string _name, string _longName) :
			name(_name),
			longName(_longName)
		{
		}
		
		Device::~Device() {
		}
		
	} // pdp8
} // ca