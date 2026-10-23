#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace toytune
{

/**
 * High-performance circular delay buffer with cubic Hermite fractional interpolation.
 */
class CircularBuffer
{
public:
    CircularBuffer() = default;

    void setSize(int newSize)
    {
        bufferSize = newSize;
        buffer.assign(bufferSize, 0.0f);
        writeIndex = 0;
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    inline void write(float sample) noexcept
    {
        buffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) % bufferSize;
    }

    /**
     * Read sample at a fractional delay back in time from the current write position.
     * Uses 4-point, 3rd-order Hermite interpolation for smooth pitch shifting.
     */
    inline float readCubic(float delaySamples) const noexcept
    {
        float readPos = static_cast<float>(writeIndex) - delaySamples;
        while (readPos < 0.0f)
            readPos += static_cast<float>(bufferSize);

        const int i1 = static_cast<int>(readPos) % bufferSize;
        const float frac = readPos - std::floor(readPos);

        const int i0 = (i1 - 1 + bufferSize) % bufferSize;
        const int i2 = (i1 + 1) % bufferSize;
        const int i3 = (i1 + 2) % bufferSize;

        const float y0 = buffer[i0];
        const float y1 = buffer[i1];
        const float y2 = buffer[i2];
        const float y3 = buffer[i3];

        // Hermite polynomial interpolation
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    int getSize() const noexcept { return bufferSize; }

private:
    std::vector<float> buffer;
    int bufferSize { 4096 };
    int writeIndex { 0 };
};

} // namespace toytune
