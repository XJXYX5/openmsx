// $Id$

#ifndef __XRENDERER_HH__
#define __XRENDERER_HH__

#ifndef	NO_X11

#include "openmsx.hh"
#include "Renderer.hh"
//#include "CharacterConverter.hh"
//#include "BitmapConverter.hh"
#include "DisplayMode.hh"
#include "VDP.hh"
#include "Keys.hh"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

namespace openmsx {

class VDP;
class VDPVRAM;
class SpriteChecker;
//class XConsole;

/** Renderer using Xlib
  */
class XRenderer : public Renderer
{
	friend int LoopCaller (void *caller) {
		reinterpret_cast <XRenderer *>(caller)->EventLoop ();
		return 0;
	}
public:


	// Renderer interface:

	void reset(const EmuTime &time) {} // TODO
	bool checkSettings();
	void frameStart(const EmuTime &time);
	void putImage(const EmuTime &time, bool store);
	void putStoredImage();
	void updateTransparency(bool enabled, const EmuTime &time);
	void updateForegroundColour(int colour, const EmuTime &time);
	void updateBackgroundColour(int colour, const EmuTime &time);
	void updateBlinkForegroundColour(int colour, const EmuTime &time);
	void updateBlinkBackgroundColour(int colour, const EmuTime &time);
	void updateBlinkState(bool enabled, const EmuTime &time);
	void updatePalette(int index, int grb, const EmuTime &time);
	void updateVerticalScroll(int scroll, const EmuTime &time);
	void updateHorizontalScrollLow(byte scroll, const EmuTime &time);
	void updateHorizontalScrollHigh(byte scroll, const EmuTime &time);
	void updateBorderMask(bool masked, const EmuTime &time);
	void updateMultiPage(bool multiPage, const EmuTime &time);
	void updateHorizontalAdjust(int adjust, const EmuTime &time);
	void updateDisplayEnabled(bool enabled, const EmuTime &time);
	void updateDisplayMode(DisplayMode mode, const EmuTime &time);
	void updateNameBase(int addr, const EmuTime &time);
	void updatePatternBase(int addr, const EmuTime &time);
	void updateColourBase(int addr, const EmuTime &time);
	void updateVRAM(int offset, const EmuTime &time);
	void updateWindow(bool enabled, const EmuTime &time);

private:

	friend class XRendererFactory;

	/** Constructor, called by XRendererFactory.
	  */
	XRenderer(RendererFactory::RendererID id, VDP *vdp);

	/** Destructor.
	  */
	virtual ~XRenderer();

	void EventLoop (void);	// new thread
	VDP *vdp;
	VDPVRAM *vram;
	// for clarity Xlib specific variables are put in a struct
	struct {
		Display *display;
		int screen;
		int depth;
		Window window;
		Pixmap pixmap;
		GC gc;
		XKeyboardState keyboard_state;
	} X;
	class convert {
	public:
		convert (KeySym sym) : sym(sym) {}
		operator SDLKey () { return (*keymap)[sym]; }
	private:
		KeySym sym;
		static map<KeySym, SDLKey> *makemap();
		static map<KeySym, SDLKey> *keymap;
	};
};

} // namespace openmsx

#endif	// !NO_X11

#endif // __XRENDERER_HH__
