// $Id$

#ifndef __CPU_HH__
#define __CPU_HH__

#include <set>
#include "openmsx.hh"
#include "EmuTime.hh"
#include "CPUTables.hh"

#ifdef DEBUG
#define CPU_DEBUG
#endif

using std::multiset;

namespace openmsx {

class BooleanSetting;
class CPUInterface;
class Scheduler;

typedef signed char offset;


class CPU : public CPUTables
{
friend class MSXCPU;
public:
	typedef union {
	#ifndef WORDS_BIGENDIAN
		struct { byte l,h; } B;
	#else
		struct { byte h,l; } B;
	#endif
		word w;
	} z80regpair;

	class CPURegs {
	public:
		z80regpair AF,  BC,  DE,  HL, IX, IY, PC, SP;
		z80regpair AF2, BC2, DE2, HL2;
		bool nextIFF1, IFF1, IFF2, HALT;
		byte IM, I;
		byte R, R2;	// refresh = R&127 | R2&128

		void ei() { IFF1 = nextIFF1 = IFF2 = true; }
		void di() { IFF1 = nextIFF1 = IFF2 = false; }
	};

	virtual ~CPU();
	
	void init(Scheduler* scheduler);
	void setInterface(CPUInterface* interf);

	/**
	 * Reset the CPU.
	 */
	void reset(const EmuTime& time);

	/**
	 * Emulated CPU till a given target-time.
	 * This implicitly calls the method setTargetTime().
	 */
	void executeUntilTarget(const EmuTime& time);

	/**
	 * Sets the CPU its current time.
	 * This is used to 'warp' a CPU when you switch between Z80/R800.
	 */
	virtual void setCurrentTime(const EmuTime& time) = 0;

	/**
	 * Returns the CPU its current time.
	 */
	virtual const EmuTime& getCurrentTime() const = 0;

	/**
	 * Alter the target time.
	 * The Scheduler uses this to announce changes in the scheduling.
	 */
	void setTargetTime(const EmuTime& time);

	/**
	 * Get the target time. Only used to switch active CPU.
	 */
	const EmuTime& getTargetTime() const;

	/**
	 * Wait 
	 */
	void wait(const EmuTime& time);

	/**
	 * Read a byte from memory. If possible the byte is read from
	 * cache, otherwise the readMem() method of CPUInterface is used.
	 */
	byte readMem(word address);

	/**
	 * Write a byte from memory. If possible the byte is written to
	 * cache, otherwise the writeMem() method of CPUInterface is used.
	 */
	void writeMem(word address, byte value);

	/**
	 * Invalidate the CPU its cache for the interval
	 * [start, start+num*CACHE_LINE_SIZE).
	 */
	void invalidateCache(word start, int num);

	/**
	 * This method raises an interrupt. A device may call this
	 * method more than once. If the device wants to lower the
	 * interrupt again it must call the lowerIRQ() method exactly as
	 * many times.
	 */
	void raiseIRQ();

	/**
	 * This methods lowers the interrupt again. A device may never
	 * call this method more often than it called the method
	 * raiseIRQ().
	 */
	void lowerIRQ();

	// cache constants
	static const int CACHE_LINE_BITS = 8;	// 256 bytes
	static const int CACHE_LINE_SIZE = 1 << CACHE_LINE_BITS;
	static const int CACHE_LINE_NUM  = 0x10000 / CACHE_LINE_SIZE;
	static const int CACHE_LINE_LOW  = CACHE_LINE_SIZE - 1;
	static const int CACHE_LINE_HIGH = 0xffff - CACHE_LINE_LOW;

protected:
	/** Create a new CPU.
	  */
	CPU();

	/** Emulate CPU till a previously set target time,
	  * the target may change (become smaller) during emulation.
	  */
	virtual void executeCore() = 0;

	/**
	  * Execute further than strictly requested by
	  * caller of executeUntilTarget() or wait(). Typically used to
	  * be able to emulate complete instruction (part of it passed
	  * the target time border).
	  */
	void extendTarget(const EmuTime& time);

	// state machine variables
	CPURegs R;
	int slowInstructions;
	int IRQStatus;

	z80regpair memptr;
	offset ofst;

	CPUInterface* interface;
	EmuTime targetTime;

	// memory cache
	const byte* readCacheLine[CACHE_LINE_NUM];
	byte* writeCacheLine[CACHE_LINE_NUM];
	bool readCacheTried [CACHE_LINE_NUM];
	bool writeCacheTried[CACHE_LINE_NUM];

	// debugger
	void doBreak();
	void doStep();
	void doContinue();
	
	static multiset<word> breakPoints;
	static bool breaked;
	static bool step;

#ifdef CPU_DEBUG
	static BooleanSetting* traceSetting;

public:
	byte debugmemory[65536];
	char to_print_string[300];
#endif

private:
	Scheduler* scheduler;
};

} // namespace openmsx
#endif //__CPU_HH__
