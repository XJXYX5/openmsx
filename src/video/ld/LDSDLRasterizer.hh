#ifndef LDSDLRASTERIZER_HH
#define LDSDLRASTERIZER_HH

#include "LDRasterizer.hh"
#include <concepts>
#include <memory>

namespace openmsx {

class OutputSurface;
class RawFrame;
class PostProcessor;

/** Rasterizer using a frame buffer approach: it writes pixels to a single
  * rectangular pixel buffer.
  */
template<std::unsigned_integral Pixel>
class LDSDLRasterizer final : public LDRasterizer
{
public:
	LDSDLRasterizer(
		OutputSurface& screen,
		std::unique_ptr<PostProcessor> postProcessor);
	~LDSDLRasterizer() override;

	// Rasterizer interface:
	[[nodiscard]] PostProcessor* getPostProcessor() const override;
	void frameStart(EmuTime::param time) override;
	void drawBlank(int r, int g, int b) override;
	[[nodiscard]] RawFrame* getRawFrame() override;

private:
	/** The video post processor which displays the frames produced by this
	  *  rasterizer.
	  */
	const std::unique_ptr<PostProcessor> postProcessor;

	/** The next frame as it is delivered by the VDP, work in progress.
	  */
	std::unique_ptr<RawFrame> workFrame;
};

} // namespace openmsx

#endif
