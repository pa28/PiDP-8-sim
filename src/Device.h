/*
**
** Device
**
** Richard Buckley July 28, 2015
**
*/

#includle <string>

namespace ca {
	namespace pdp8 {
		class Device {
		public:
			Device(string name, string longName);
			virtual ~Device();
		
		protected:
			string	name, longName;
		};
	} // pdp8
} // ca