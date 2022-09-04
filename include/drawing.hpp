//
// Copyright (c) 2022 Christopher Gassib
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CLGDRAWING_HPP
#define CLGDRAWING_HPP

namespace clg
{
    uint32_t *pFrameBuf;

    int GetCompressedTextureLinePitch(int width)
    {
        const auto result = round_up_to_alignment<sizeof(uint16_t)>(width);
        return result;
    }

    void CompressTexture(int width, int height, const uint8_t* const uncompressed, int srcLinePitch, uint8_t* const compressed, int dstLinePitch)
    {
        auto pSrc = uncompressed;
        auto pDst = compressed;
        for (auto y = 0; y < height; y++)
        {
            const auto pNextSrcLine = pSrc + srcLinePitch;
            const auto pNextDstLine = pDst + dstLinePitch;

            for (auto x = 0; x < width; x += 4)
            {
                if (x + 3 < width)
                {
                    uint8_t packed = *pSrc << 6 & 0xc0; pSrc++;
                    packed |= *pSrc << 4 & 0x30; pSrc++;
                    packed |= *pSrc << 2 & 0x0c; pSrc++;
                    packed |= *pSrc & 0x03; pSrc++;
                    *pDst++ = packed;
                }
                else if (x + 2 < width)
                {
                    uint8_t packed = *pSrc << 6 & 0xc0; pSrc++;
                    packed |= *pSrc << 4 & 0x30; pSrc++;
                    packed |= *pSrc << 2 & 0x0c; pSrc++;
                    *pDst++ = packed;
                }
                else if (x + 1 < width)
                {
                    uint8_t packed = *pSrc << 6 & 0xc0; pSrc++;
                    packed |= *pSrc << 4 & 0x30; pSrc++;
                    *pDst++ = packed;
                }
                else
                {
                    uint8_t packed = *pSrc << 6 & 0xc0; pSrc++;
                    *pDst++ = packed;
                }
            }

            pSrc = pNextSrcLine;
            pDst = pNextDstLine;
        }
    }

    inline constexpr uint8_t FetchTextureIndex(const uint8_t* const compressedTexture, int linePitch, int x, int y)
    {
        uint8_t result = compressedTexture[y * linePitch + (x >> 2)];
        result >>= 6 - ((x & 3) << 1);
        return result & 3;
    }

    // Find the x coordinate where horizontal line at y intersects the line segment from a to b
    // returns: false if the line y does not indersect the line segment a-b
    inline bool FindIntersection(const float y, const point a, const point b, float& x)
    {
//        const auto y_bounds = std::minmax(a.y, b.y);
        if ((y < a.y && y < b.y) ||
            (y > a.y && y > b.y))
        {
            return false;
        }

        if (0.0f == std::abs(a.x - b.x)) // if (it's a vertical line)
        {
            x = a.x;
            return true;
        }

        point l, r;
        if (a.x < b.x)
        {
            l = a;
            r = b;
        }
        else
        {
            l = b;
            r = a;
        }

        const float slope = (r.y - l.y) / (r.x - l.x);
        const float y_intercept = -slope * l.x + l.y;
        x = (-y_intercept + y) / slope;
        return true;
    }

    inline constexpr int FlipY(const int y)
    {
        return (pd::LcdHeight - 1) - y;
    }

    inline void WritePixel(const int x, const int y, const uint8_t pixel) // TODO: fix callers
    {
        if (0 == (2 & pixel)) return; // if (transparent)
        const auto idx = FlipY(y) * pd::LcdRowStride + (x >> 3); // (invert y-axis) * (screenLinePitch) + (x / 8)
        const auto mask = 0x80u >> (x & 0x7); // get a 1 bit mask for the desired pixel

        // overwrite display pixel block
        auto& pixelBlock = reinterpret_cast<uint8_t*>(pFrameBuf)[idx];
        pixelBlock = 0 == (pixel & 0xff) ? pixelBlock & ~mask : pixelBlock | mask;
    }

    void BlitRectangle(
        const clg::pointi& dst,                         // rectangle destination on screen (centered on this coordinate)
        const clg::recti& src,                          // start of the rectangle in pixel buffer; width and height of the rectangle in the pixel buffer
        const clg::pointi& srcCenter,                   // center of the source image (the point it renders around & rotates around)
        const int srcLinePitch,                         // line pitch of the pixel buffer
        const uint8_t* const pixels,                    // pixel buffer
        const uint_fast32_t* const debugOutlineColor    // [optional] debug outline box
    )
    {
        const clg::pointi leftBottom = dst - srcCenter;

        if (
            leftBottom.x >= pd::LcdWidth ||     // clipped off the right
            leftBottom.y>= pd::LcdHeight ||     // clipped off the bottom
            leftBottom.x+ src.width() <= 0 ||   // clipped off the left
            leftBottom.y+ src.height() <= 0     // clipped off the top
            )
        {
            return;
        }

        int dx = leftBottom.x;
        int dy = leftBottom.y;
        int dw = src.width();
        int dh = src.height();
        int sx = src.x();
        int sy = src.y();

        if (leftBottom.x < 0) // clip left
        {
            dx = 0;
            dw += leftBottom.x;
            sx = 0;
        }

        if (leftBottom.x + src.width() > pd::LcdWidth) // clip right
        {
            dw -= leftBottom.x + src.width() - pd::LcdWidth;
        }

        if (leftBottom.y < 0) // clip top
        {
            dy = 0;
            dh += leftBottom.y;
            sy = 0;
        }

        if (leftBottom.y + src.height() > pd::LcdHeight) // clip bottom
        {
            dh -= leftBottom.y + src.height() - pd::LcdHeight;
        }

        for (int i = 0; i < dh; i++)
        {
            for (int j = 0; j < dw; j++)
            {
                const auto fragment = FetchTextureIndex(pixels, srcLinePitch, sx + j, sy + i);
                WritePixel(dx + j, dy + i, fragment);
            }
        }
    }

    template<typename base_type, int fractional_bit_count>
    inline constexpr base_type make_fixed_point(float number)
    {
        constexpr int base_bit_count = sizeof(base_type) * 8;
        constexpr int whole_bit_count = base_bit_count - fractional_bit_count;
        constexpr int fractional_part_multiplier = 1 << fractional_bit_count;
        static_assert(fractional_bit_count < base_bit_count, "too many fractional bits");
        assert(number < powf(2, whole_bit_count - 1) && number > -powf(2, whole_bit_count - 1) - 1.0f);

        const base_type result = number * fractional_part_multiplier;
        return result;
    }

    template<typename base_type, int fractional_bit_count>
    inline constexpr base_type get_integer_part(base_type fixed_point)
    {
        const base_type result = fixed_point >> fractional_bit_count;
        return result;
    }

    template<typename base_type, int fractional_bit_count>
    inline constexpr float get_fractional_part(base_type fixed_point)
    {
        constexpr int fractional_part_multiplier = 1 << fractional_bit_count;
        const auto result = static_cast<float>(fixed_point) / fractional_part_multiplier;
        return result;
    }

    inline constexpr point rotate_counter_clockwise(float cosTheta, float sinTheta, const point& p)
    {
        const point result(p[0] * cosTheta - p[1] * sinTheta, p[0] * sinTheta + p[1] * cosTheta);
        return result;
    }

    // scale -> rotate -> translate
    void BlitTransformedAlphaTexturedRectangle(
        const point& dst,                               // rectangle destination on screen (centered on this coordinate)
        const sizev& scale,                             // scale on screen
        const float angle,                              // rotation in radians
        const recti& src,                               // start of the rectangle in pixel buffer; width and height of the rectangle in the pixel buffer
        const point& srcCenter,                         // center of the source image (the point it renders around & rotates around)
        const uint8_t* const pixels,                    // pixel buffer
        const int srcLinePitch,                         // line pitch of the pixel buffer
        const uint_fast32_t* const debugOutlineColor    // [optional] debug outline box
    )
    {
        const auto srcSize = src.size();
        if (srcSize.width <= 0 || srcSize.height <= 0 || scale.width <= 0.0f || scale.height <= 0.0f) // if (the scale in any dimension == 0)
        {
            return;
        }

        // get scaled dst size
        const sizev srcSizef(srcSize);
        const sizev dstSize(srcSizef * scale);

        // get scaled dst center point
        const point dstScaledCenter(srcCenter * static_cast<point>(scale));

        // get vertices of the scaled src in screen-space, centered about the origin
        const point olb(-dstScaledCenter.x, -dstScaledCenter.y);
        const point orb(dstSize.width - dstScaledCenter.x, -dstScaledCenter.y);
        const point olt(-dstScaledCenter.x, dstSize.height - dstScaledCenter.y);
        const point ort(dstSize.width - dstScaledCenter.x, dstSize.height - dstScaledCenter.y);

        // bite off those trig functions
        const auto cosTheta = cos_lookup(angle);
        const auto sinTheta = sin_lookup(angle);

        // get vertices of scaled, rotated, and translated src in screen-space
        const point lb(rotate_counter_clockwise(cosTheta, sinTheta, olb) + dst);
        const point rb(rotate_counter_clockwise(cosTheta, sinTheta, orb) + dst);
        const point lt(rotate_counter_clockwise(cosTheta, sinTheta, olt) + dst);
        const point rt(rotate_counter_clockwise(cosTheta, sinTheta, ort) + dst);
        vec2 srcScanlineNormal(cosTheta, -sinTheta);

        // get first and last scanline containing transformed dst rectangle
        int begin_scanline;
        int end_scanline;
        {
            const auto yb = std::minmax({ lb.y, rb.y, lt.y, rt.y });
            begin_scanline = clamp(static_cast<int>(std::round(yb.first)), 0, pd::LcdHeight);
            end_scanline = clamp(static_cast<int>(std::round(yb.first + (yb.second - yb.first))), 0, pd::LcdHeight);
        }

        const sizev texelsPerPixel(srcSizef / dstSize);
        for (int scanline_y = begin_scanline; scanline_y != end_scanline; scanline_y++) // ~1 - 2.9ms for 240 scanlines
        {
            // figure out where the current scanline intercepts each of the source image edges
            // along with the src texture coordinates of these intercepts
            const float centerScanline = scanline_y + 0.5f;
            float x_intercepts[4];
            vec2 src_intercepts[4];
            int c = 0;
            {
                float x;
                if (FindIntersection(centerScanline, lb, rb, x))
                {
                    x_intercepts[c] = x;
                    const auto pX = x - lb.x;               // distance from x_intercept to left-bottom.x
                    const auto pY = centerScanline - lb.y;  // distance from y_intercept to left-bottom.y
                    const auto pixelDistance = pX * cosTheta + pY * sinTheta;
                    src_intercepts[c] = vec2(pixelDistance * texelsPerPixel.width, 0.0f);
                    c++;
                }
                if (FindIntersection(centerScanline, lb, lt, x))
                {
                    x_intercepts[c] = x;
                    const auto pX = x - lb.x;               // distance from x_intercept to left-bottom.x
                    const auto pY = centerScanline - lb.y;  // distance from y_intercept to left-bottom.y
                    const auto pixelDistance = pX * -sinTheta + pY * cosTheta;
                    src_intercepts[c] = vec2(0.0f, pixelDistance * texelsPerPixel.height);
                    c++;
                }
                if (FindIntersection(centerScanline, lt, rt, x))
                {
                    x_intercepts[c] = x;
                    const auto pX = x - lt.x;               // distance from x_intercept to left-top.x
                    const auto pY = centerScanline - lt.y;  // distance from y_intercept to left-top.y
                    const auto pixelDistance = pX * cosTheta + pY * sinTheta;
                    src_intercepts[c] = vec2(pixelDistance * texelsPerPixel.width, srcSizef.height);
                    c++;
                }
                if (FindIntersection(centerScanline, rb, rt, x)) // rb, rt
                {
                    x_intercepts[c] = x;
                    const auto pX = x - rb.x;               // distance from x_intercept to right-bottom.x
                    const auto pY = centerScanline - rb.y;  // distance from y_intercept to right-bottom.y
                    const auto pixelDistance = pX * -sinTheta + pY * cosTheta;
                    src_intercepts[c] = vec2(srcSizef.width, pixelDistance * texelsPerPixel.height);
                    c++;
                }

                assert(c == 2);
            }

            const auto mm = std::minmax_element(&x_intercepts[0], &x_intercepts[c]);
            const auto& first_x_intercept = *(mm.first);
            const auto& second_x_intercept = *(mm.second);
            const int begin_column = clamp(static_cast<int>(std::round(first_x_intercept)), 0, pd::LcdWidth);
            const int end_column = clamp(static_cast<int>(std::round(second_x_intercept)), 0, pd::LcdWidth);
            if (begin_column >= end_column)
            {
                continue;
            }

            const auto& srcStart = src_intercepts[mm.first - &x_intercepts[0]]; // first intercept in src-space
            const auto& srcEnd = src_intercepts[mm.second - &x_intercepts[0]]; // last intercept in src-space
            const auto srcScanlineLength = (srcEnd - srcStart).length();

            // calculate src texture coordinates
            const float invDstScanlineLength = 1.0f / (second_x_intercept - first_x_intercept);
            const float scanlineRelativeProgress = clamp(((begin_column + 0.5f) - first_x_intercept) * invDstScanlineLength);

            vec2 srcPos(srcStart + srcScanlineNormal * (srcScanlineLength * scanlineRelativeProgress));
            int32_t srcPosX = make_fixed_point<int32_t, 24>(srcPos.x);
            int32_t srcPosY = make_fixed_point<int32_t, 24>(srcPos.y);

            const vec2 srcStep(srcScanlineNormal * (srcScanlineLength * invDstScanlineLength));
            const int32_t srcStepX = make_fixed_point<int32_t, 24>(srcStep.x);
            const int32_t srcStepY = make_fixed_point<int32_t, 24>(srcStep.y);

            for (int column_x = begin_column;;) // ???ms (/wo FetchTextureIndex() or WritePixel())
            {
                assert(scanline_y >= 0 && scanline_y < pd::LcdHeight && column_x >= 0 && column_x < pd::LcdWidth);

                // fetch texture fragment
                const auto fragment = FetchTextureIndex( // ~5ms (/wo WritePixel ?? - ??ms) (/w WritePixe() 16 - 17.9ms)
                    pixels,
                    srcLinePitch,
                    get_integer_part<int32_t, 24>(srcPosX),
                    get_integer_part<int32_t, 24>(srcPosY)
                    );

                // write pixel
                WritePixel(column_x, scanline_y, fragment); // ~7ms (/wo FetchTextureIndex() ?? - ??ms) (/w FetchTextureIndex() 16 - 17.9ms)

                if (++column_x == end_column)
                {
                    break;
                }

                // advance src image scanline
                srcPosX += srcStepX;
                srcPosY += srcStepY;
            }
        }
    }

    void DrawAxisAlignedBitmap(
        const uint8_t* const pixels,                    // source bitmap's buffer to blit from
        const uint_fast32_t srcLinePitch,               // line pitch (in bytes) of the source bitmap buffer
        const recti& src,                               // rectangle inside the source bitmap to render
        const pointi& dst,                              // display location to render to (where the center point is rendered)
        const pointi& srcCenterOffset,                  // center of of the source rectangle (the point rendered at dst)
        const uint_fast32_t* const debugOutlineColor    // [optional] debug outline box
    )
    {
        BlitRectangle(
            dst,
            src,
            srcCenterOffset,
            srcLinePitch,
            pixels,
            debugOutlineColor
        );
    }

    void DrawBitmap(
        const uint8_t* const pixels,                    // source bitmap's buffer to blit from
        const uint_fast32_t srcLinePitch,               // line pitch (in bytes) of the source bitmap buffer
        const recti& src,                               // rectangle inside the source bitmap to render
        const point& dst,                               // display location to render to (where the center point is rendered)
        const float orientation,                        // angle of rotation (in radians)
        const sizev& scale,                             // scale to render the bitmap at
        const point& srcCenterOffset,                   // center of of the source rectangle (the point rendered at dst)
        const uint_fast32_t* const debugOutlineColor    // [optional] debug outline box
    )
    {
        BlitTransformedAlphaTexturedRectangle(
            dst,
            scale,
            orientation,
            src,
            srcCenterOffset,
            pixels,
            srcLinePitch,
            debugOutlineColor
        );
    }
} // namespace clg

#endif // CLGDRAWING_HPP
