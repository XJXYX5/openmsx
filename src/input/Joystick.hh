// $Id$

#ifndef __JOYSTICK_HH__
#define __JOYSTICK_HH__

#include "JoystickDevice.hh"
#include "EventListener.hh"
#include "MSXException.hh"
#include <SDL/SDL.h>


namespace openmsx {

class JoystickException : public MSXException {
	public:
		JoystickException(const string &desc) : MSXException(desc) {}
};

class Joystick : public JoystickDevice, EventListener
{
	public:
		Joystick(int joyNum);
		virtual ~Joystick();

		//Pluggable
		virtual const string &getName() const;
		virtual void plug(Connector* connector, const EmuTime& time);
		virtual void unplug(const EmuTime& time);

		//JoystickDevice
		virtual byte read(const EmuTime &time);
		virtual void write(byte value, const EmuTime &time);

		//EventListener
		virtual bool signalEvent(SDL_Event &event);

	private:
		static const int THRESHOLD = 32768/10;

		string name;

		int joyNum;
		SDL_Joystick *joystick;
		byte status;
};

} // namespace openmsx
#endif
