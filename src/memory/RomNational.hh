// $Id$

#ifndef __ROMNATIONAL_HH__
#define __ROMNATIONAL_HH__

#include "Rom16kBBlocks.hh"
#include "SRAM.hh"


namespace openmsx {

class RomNational : public Rom16kBBlocks
{
	public:
		RomNational(Device* config, const EmuTime &time, Rom *rom);
		virtual ~RomNational();
		
		virtual void reset(const EmuTime &time);
		virtual byte readMem(word address, const EmuTime &time);
		virtual const byte* getReadCacheLine(word address) const;
		virtual void writeMem(word address, byte value,
		                      const EmuTime &time);
		virtual byte* getWriteCacheLine(word address) const;
	
	private:
		byte control;
		byte bankSelect[4];
		SRAM sram;
		int sramAddr;
};

} // namespace openmsx

#endif
