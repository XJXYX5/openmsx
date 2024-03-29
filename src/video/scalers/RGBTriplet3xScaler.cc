#include "RGBTriplet3xScaler.hh"
#include "SuperImposedVideoFrame.hh"
#include "LineScalers.hh"
#include "RawFrame.hh"
#include "ScalerOutput.hh"
#include "RenderSettings.hh"
#include "enumerate.hh"
#include "vla.hh"
#include "build-info.hh"
#include <array>
#include <cstdint>

namespace openmsx {

template<std::unsigned_integral Pixel>
RGBTriplet3xScaler<Pixel>::RGBTriplet3xScaler(
		const PixelOperations<Pixel>& pixelOps_,
		const RenderSettings& renderSettings)
	: Scaler3<Pixel>(pixelOps_)
	, pixelOps(pixelOps_)
	, scanline(pixelOps_)
	, settings(renderSettings)
{
}

template<std::unsigned_integral Pixel>
std::pair<unsigned, unsigned> RGBTriplet3xScaler<Pixel>::calcBlur()
{
	unsigned c1 = settings.getBlurFactor();
	unsigned c2 = (3 * 256) - (2 * c1);
	return {c1, c2};
}

[[nodiscard]] static inline std::pair<unsigned, unsigned> calcSpill(unsigned c1, unsigned c2, unsigned x)
{
	unsigned r = (c2 * x) >> 8;
	unsigned s = (c1 * x) >> 8;
	if (r > 255) {
		s += (r - 255) / 2;
		r = 255;
	}
	return {r, s};
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::rgbify(
	std::span<const Pixel> in, std::span<Pixel> out, unsigned c1, unsigned c2)
{
	assert((3 * in.size()) == out.size());
	auto inSize = in.size();
	size_t i = 0;

	auto    [r, rs] = calcSpill(c1, c2, pixelOps.red256  (in[i + 0]));
	auto    [g, gs] = calcSpill(c1, c2, pixelOps.green256(in[i + 0]));
	out[3 * i + 0] = pixelOps.combine256(r , gs, 0);
	auto    [b, bs] = calcSpill(c1, c2, pixelOps.blue256 (in[i + 0]));
	out[3 * i + 1] = pixelOps.combine256(rs, g , bs);
	std::tie(r, rs) = calcSpill(c1, c2, pixelOps.red256  (in[i + 1]));
	out[3 * i + 2] = pixelOps.combine256(rs, gs, b);

	for (++i; i < (inSize - 1); ++i) {
		std::tie(g, gs) = calcSpill(c1, c2, pixelOps.green256(in[i + 0]));
		out[3 * i + 0] = pixelOps.combine256(r , gs, bs);
		std::tie(b, bs) = calcSpill(c1, c2, pixelOps.blue256 (in[i + 0]));
		out[3 * i + 1] = pixelOps.combine256(rs, g , bs);
		std::tie(r, rs) = calcSpill(c1, c2, pixelOps.red256  (in[i + 1]));
		out[3 * i + 2] = pixelOps.combine256(rs, gs, b);
	}

	std::tie(g, gs) = calcSpill(c1, c2, pixelOps.green256(in[i + 0]));
	out[3 * i + 0] = pixelOps.combine256(r , gs, bs);
	std::tie(b, bs) = calcSpill(c1, c2, pixelOps.blue256 (in[i + 0]));
	out[3 * i + 1] = pixelOps.combine256(rs, g , bs);
	out[3 * i + 2] = pixelOps.combine256(0 , gs, b);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scaleLine(
	std::span<const Pixel> srcLine, std::span<Pixel> dstLine, PolyLineScaler<Pixel>& scale,
	unsigned c1, unsigned c2)
{
	if (scale.isCopy()) {
		rgbify(srcLine, dstLine, c1, c2);
	} else {
		VLA_SSE_ALIGNED(Pixel, tmp, dstLine.size() / 3);
		scale(srcLine, tmp);
		rgbify(tmp, dstLine, c1, c2);
	}
}

// Note: the idea is that this method RGBifies a line that is first scaled
// to output-width / 3. So, when calling this, keep this in mind and pass a
// scale functor that scales the input with correctly.
template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::doScale1(FrameSource& src,
	unsigned srcStartY, unsigned /*srcEndY*/, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY,
	PolyLineScaler<Pixel>& scale)
{
	VLA_SSE_ALIGNED(Pixel, buf, srcWidth);

	auto [c1, c2] = calcBlur();

	unsigned dstWidth = dst.getWidth();
	int scanlineFactor = settings.getScanlineFactor();
	unsigned y = dstStartY;
	auto srcLine = src.getLine(srcStartY++, buf);
	auto dstLine0 = dst.acquireLine(y + 0);
	scaleLine(srcLine, dstLine0, scale, c1, c2);

	Scale_1on1<Pixel> copy;
	auto dstLine1 = dst.acquireLine(y + 1);
	copy(dstLine0, dstLine1);

	for (/* */; (y + 4) < dstEndY; y += 3, srcStartY += 1) {
		srcLine = src.getLine(srcStartY, buf);
		auto dstLine3 = dst.acquireLine(y + 3);
		scaleLine(srcLine, dstLine3, scale, c1, c2);

		auto dstLine4 = dst.acquireLine(y + 4);
		copy(dstLine3, dstLine4);

		auto dstLine2 = dst.acquireLine(y + 2);
		scanline.draw(dstLine0, dstLine3,
		              dstLine2, scanlineFactor);

		dst.releaseLine(y + 0, dstLine0);
		dst.releaseLine(y + 1, dstLine1);
		dst.releaseLine(y + 2, dstLine2);
		dstLine0 = dstLine3;
		dstLine1 = dstLine4;
	}

	srcLine = src.getLine(srcStartY, buf);
	VLA_SSE_ALIGNED(Pixel, buf2, dstWidth);
	scaleLine(srcLine, buf2, scale, c1, c2);
	auto dstLine2 = dst.acquireLine(y + 2);
	scanline.draw(dstLine0, buf2, dstLine2, scanlineFactor);
	dst.releaseLine(y + 0, dstLine0);
	dst.releaseLine(y + 1, dstLine1);
	dst.releaseLine(y + 2, dstLine2);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::doScale2(FrameSource& src,
	unsigned srcStartY, unsigned /*srcEndY*/, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY,
	PolyLineScaler<Pixel>& scale)
{
	VLA_SSE_ALIGNED(Pixel, buf, srcWidth);
	auto [c1, c2] = calcBlur();

	int scanlineFactor = settings.getScanlineFactor();
	for (unsigned srcY = srcStartY, dstY = dstStartY; dstY < dstEndY;
	     srcY += 2, dstY += 3) {
		auto srcLine0 = src.getLine(srcY + 0, buf);
		auto dstLine0 = dst.acquireLine(dstY + 0);
		scaleLine(srcLine0, dstLine0, scale, c1, c2);

		auto srcLine1 = src.getLine(srcY + 1, buf);
		auto dstLine2 = dst.acquireLine(dstY + 2);
		scaleLine(srcLine1, dstLine2, scale, c1, c2);

		auto dstLine1 = dst.acquireLine(dstY + 1);
		scanline.draw(dstLine0, dstLine2,
		              dstLine1, scanlineFactor);

		dst.releaseLine(dstY + 0, dstLine0);
		dst.releaseLine(dstY + 1, dstLine1);
		dst.releaseLine(dstY + 2, dstLine2);
	}
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale2x1to9x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_2on3<Pixel>> op(pixelOps);
	doScale1(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale2x2to9x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_2on3<Pixel>> op(pixelOps);
	doScale2(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale1x1to3x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_1on1<Pixel>> op;
	doScale1(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale1x2to3x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_1on1<Pixel>> op;
	doScale2(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale4x1to9x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_4on3<Pixel>> op(pixelOps);
	doScale1(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale4x2to9x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_4on3<Pixel>> op(pixelOps);
	doScale2(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale2x1to3x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_2on1<Pixel>> op(pixelOps);
	doScale1(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale2x2to3x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_2on1<Pixel>> op(pixelOps);
	doScale2(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale8x1to9x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_8on3<Pixel>> op(pixelOps);
	doScale1(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale8x2to9x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_8on3<Pixel>> op(pixelOps);
	doScale2(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale4x1to3x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_4on1<Pixel>> op(pixelOps);
	doScale1(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scale4x2to3x3(FrameSource& src,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	PolyScale<Pixel, Scale_4on1<Pixel>> op(pixelOps);
	doScale2(src, srcStartY, srcEndY, srcWidth,
	         dst, dstStartY, dstEndY, op);
}

template<std::unsigned_integral Pixel>
static constexpr void fillLoop(std::span<Pixel, 9> in, std::span<Pixel> out)
{
	size_t outSize = out.size();
	assert(outSize >= 6);
	assert((outSize % 3) == 0);

	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	auto in3 = in[3];
	auto in4 = in[4];
	auto in5 = in[5];
	for (unsigned x = 3; x < (outSize - 3); x += 3) {
		out[x + 0] = in3;
		out[x + 1] = in4;
		out[x + 2] = in5;
	}
	out[outSize - 3] = in[6];
	out[outSize - 2] = in[7];
	out[outSize - 1] = in[8];
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scaleBlank1to3(
		FrameSource& src, unsigned srcStartY, unsigned srcEndY,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	auto [c1, c2] = calcBlur();
	int scanlineFactor = settings.getScanlineFactor();

	unsigned dstHeight = dst.getHeight();
	unsigned stopDstY = (dstEndY == dstHeight)
	                  ? dstEndY : dstEndY - 3;
	unsigned srcY = srcStartY, dstY = dstStartY;
	for (/* */; dstY < stopDstY; srcY += 1, dstY += 3) {
		auto color = src.getLineColor<Pixel>(srcY);

		std::array<Pixel, 3>     inNormal;
		std::array<Pixel, 3 * 3> outNormal;
		std::array<Pixel, 3 * 3> outScanline;
		inNormal[0] = inNormal[1] = inNormal[2] = color;
		rgbify(inNormal, outNormal, c1, c2);
		for (auto [i, out] : enumerate(outScanline)) {
			out = scanline.darken(outNormal[i], scanlineFactor);
		}

		auto dstLine0 = dst.acquireLine(dstY + 0);
		fillLoop(std::span{outNormal}, dstLine0);
		dst.releaseLine(dstY + 0, dstLine0);

		auto dstLine1 = dst.acquireLine(dstY + 1);
		fillLoop(std::span{outNormal}, dstLine1);
		dst.releaseLine(dstY + 1, dstLine1);

		auto dstLine2 = dst.acquireLine(dstY + 2);
		fillLoop(std::span{outScanline}, dstLine2);
		dst.releaseLine(dstY + 2, dstLine2);
	}
	if (dstY != dstHeight) {
		unsigned nextLineWidth = src.getLineWidth(srcY + 1);
		assert(src.getLineWidth(srcY) == 1);
		assert(nextLineWidth != 1);
		this->dispatchScale(src, srcY, srcEndY, nextLineWidth,
		                    dst, dstY, dstEndY);
	}
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scaleBlank2to3(
		FrameSource& src, unsigned srcStartY, unsigned /*srcEndY*/,
		ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	auto [c1, c2] = calcBlur();
	int scanlineFactor = settings.getScanlineFactor();
	for (unsigned srcY = srcStartY, dstY = dstStartY;
	     dstY < dstEndY; srcY += 2, dstY += 3) {
		auto color0 = src.getLineColor<Pixel>(srcY + 0);
		auto color1 = src.getLineColor<Pixel>(srcY + 1);

		std::array<Pixel, 3> inNormal;
		std::array<Pixel, 3 * 3> out0Normal;
		std::array<Pixel, 3 * 3> out1Normal;
		std::array<Pixel, 3 * 3> outScanline;

		inNormal[0] = inNormal[1] = inNormal[2] = color0;
		rgbify(inNormal, out0Normal, c1, c2);
		inNormal[0] = inNormal[1] = inNormal[2] = color1;
		rgbify(inNormal, out1Normal, c1, c2);

		for (auto [i, out] : enumerate(outScanline)) {
			out = scanline.darken(
				out0Normal[i], out1Normal[i],
				scanlineFactor);
		}

		auto dstLine0 = dst.acquireLine(dstY + 0);
		fillLoop(std::span{out0Normal}, dstLine0);
		dst.releaseLine(dstY + 0, dstLine0);

		auto dstLine1 = dst.acquireLine(dstY + 1);
		fillLoop(std::span{outScanline}, dstLine1);
		dst.releaseLine(dstY + 1, dstLine1);

		auto dstLine2 = dst.acquireLine(dstY + 2);
		fillLoop(std::span{out1Normal}, dstLine2);
		dst.releaseLine(dstY + 2, dstLine2);
	}
}

template<std::unsigned_integral Pixel>
void RGBTriplet3xScaler<Pixel>::scaleImage(FrameSource& src, const RawFrame* superImpose,
	unsigned srcStartY, unsigned srcEndY, unsigned srcWidth,
	ScalerOutput<Pixel>& dst, unsigned dstStartY, unsigned dstEndY)
{
	if (superImpose) {
		SuperImposedVideoFrame<Pixel> sf(src, *superImpose, pixelOps);
		srcWidth = sf.getLineWidth(srcStartY);
		this->dispatchScale(sf,  srcStartY, srcEndY, srcWidth,
		                    dst, dstStartY, dstEndY);
	} else {
		this->dispatchScale(src, srcStartY, srcEndY, srcWidth,
		                    dst, dstStartY, dstEndY);
	}
}

// Force template instantiation.
#if HAVE_16BPP
template class RGBTriplet3xScaler<uint16_t>;
#endif
#if HAVE_32BPP
template class RGBTriplet3xScaler<uint32_t>;
#endif

} // namespace openmsx
