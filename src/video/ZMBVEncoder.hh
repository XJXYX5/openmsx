// Code based on DOSBox-0.65

#ifndef ZMBVENCODER_HH
#define ZMBVENCODER_HH

#include "PixelFormat.hh"
#include "MemBuffer.hh"
#include "aligned.hh"
#include <concepts>
#include <cstdint>
#include <span>
#include <string_view>
#include <zlib.h>

namespace openmsx {

class FrameSource;
template<std::unsigned_integral P> class PixelOperations;

class ZMBVEncoder
{
public:
	static constexpr std::string_view CODEC_4CC = "ZMBV";

	ZMBVEncoder(unsigned width, unsigned height, unsigned bpp);

	[[nodiscard]] std::span<const uint8_t> compressFrame(bool keyFrame, FrameSource* frame);

private:
	enum Format {
		ZMBV_FORMAT_16BPP = 6,
		ZMBV_FORMAT_32BPP = 8
	};

	void setupBuffers(unsigned bpp);
	[[nodiscard]] unsigned neededSize() const;
	template<std::unsigned_integral P> void addFullFrame(const PixelFormat& pixelFormat, unsigned& workUsed);
	template<std::unsigned_integral P> void addXorFrame (const PixelFormat& pixelFormat, unsigned& workUsed);
	template<std::unsigned_integral P> [[nodiscard]] unsigned possibleBlock(int vx, int vy, size_t offset);
	template<std::unsigned_integral P> [[nodiscard]] unsigned compareBlock(int vx, int vy, size_t offset);
	template<std::unsigned_integral P> void addXorBlock(
		const PixelOperations<P>& pixelOps, int vx, int vy,
		size_t offset, unsigned& workUsed);
	[[nodiscard]] const void* getScaledLine(FrameSource* frame, unsigned y, void* workBuf) const;

private:
	MemBuffer<uint8_t, SSE_ALIGNMENT> oldFrame;
	MemBuffer<uint8_t, SSE_ALIGNMENT> newFrame;
	MemBuffer<uint8_t, SSE_ALIGNMENT> work;
	MemBuffer<uint8_t> output;
	MemBuffer<size_t> blockOffsets;
	unsigned outputSize;

	z_stream zstream;

	const unsigned width;
	const unsigned height;
	size_t pitch;
	unsigned pixelSize;
	Format format;
};

} // namespace openmsx

#endif
