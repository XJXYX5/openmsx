// $Id$

#ifndef __OSDCONSOLERENDERER_HH__
#define __OSDCONSOLERENDERER_HH__

#include "ConsoleRenderer.hh"
#include "Settings.hh"
#include "DummyFont.hh"

#ifdef HAVE_SDL_IMAGE_H
#include "SDL_image.h"
#else
#include "SDL/SDL_image.h"
#endif

#include <string>
using std::string;


namespace openmsx {

class OSDConsoleRenderer;
class Console;
class FileContext;


class BackgroundSetting : public FilenameSetting
{
	public:
		BackgroundSetting(OSDConsoleRenderer *console, const std::string settingName,
		                  const string &filename);

		virtual bool checkFile(const string &filename);

	private:
		OSDConsoleRenderer *console;
};

class FontSetting : public FilenameSetting
{
	public:
		FontSetting(OSDConsoleRenderer *console, const std::string settingName,
		            const string &filename);

		virtual bool checkFile(const string &filename);

	private:
		OSDConsoleRenderer *console;
};

class OSDConsoleRenderer : public ConsoleRenderer
{
	public:
		OSDConsoleRenderer(Console *console_);
		virtual ~OSDConsoleRenderer();
		virtual bool loadBackground(const string &filename) = 0;
		virtual bool loadFont(const string &filename) = 0;
		virtual void drawConsole() = 0;

		enum Placement {
			CP_TOPLEFT,    CP_TOP,    CP_TOPRIGHT,
			CP_LEFT,       CP_CENTER, CP_RIGHT,
			CP_BOTTOMLEFT, CP_BOTTOM, CP_BOTTOMRIGHT
		};
		void setBackgroundName(const string &name);
		void setFontName(const string &name);

	protected:
		void updateConsoleRect(SDL_Rect & rect);
		void initConsoleSize(void);

		/** How transparent is the console? (0=invisible, 255=opaque)
		  * Note that when using a background image on the GLConsole,
		  * that image's alpha channel is used instead.
		  */
		static const int CONSOLE_ALPHA = 180;
		static const int BLINK_RATE = 500;
		static const int CHAR_BORDER = 4;

		static Placement consolePlacement;
		static string fontName;
		static string backgroundName;

		int consoleRows;
		int consoleColumns;
		EnumSetting<Placement> *consolePlacementSetting;
		IntegerSetting *consoleRowsSetting;
		IntegerSetting *consoleColumnsSetting;
		class Font *font;
		FileContext *context;
		bool blink;
		unsigned lastBlinkTime;
		int lastCursorPosition;

	private:
		void adjustColRow();

		static int wantedColumns;
		static int wantedRows;
		int currentMaxX;
		int currentMaxY;
		Console *console;
};

} // namespace openmsx

#endif
