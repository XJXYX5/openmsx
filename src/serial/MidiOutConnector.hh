// $Id$

#ifndef __MIDIOUTCONNECTOR_HH__
#define __MIDIOUTCONNECTOR_HH__

#include "Connector.hh"
#include "SerialDataInterface.hh"

namespace openmsx {

class DummyMidiOutDevice;
class MidiOutLogger;

class MidiOutConnector : public Connector, public SerialDataInterface
{
	public:
		MidiOutConnector(const string& name, const EmuTime& time);
		virtual ~MidiOutConnector();
		
		// Connector 
		virtual const string& getName() const;
		virtual const string& getClass() const;
		virtual void plug(Pluggable* device, const EmuTime &time);
		virtual void unplug(const EmuTime& time);

		// SerialDataInterface
		virtual void setDataBits(DataBits bits);
		virtual void setStopBits(StopBits bits);
		virtual void setParityBit(bool enable, ParityBit parity);
		virtual void recvByte(byte value, const EmuTime& time);
	
	private:
		string name;
		DummyMidiOutDevice* dummy;

		MidiOutLogger* logger;
};

} // namespace openmsx

#endif
