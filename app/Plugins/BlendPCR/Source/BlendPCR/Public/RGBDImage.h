#pragma once

#include <string>

struct RGBDImage {
    std::string cameraId;

    uint16_t width = 640;
    uint16_t height = 576;

    uint16_t* depth = nullptr;
    uint32_t* color = nullptr;
    float* lookup2D = nullptr;

    FMatrix44f modelMatrix;

    ~RGBDImage() {
        delete[] depth;
        delete[] color;
    }

    void get3DCoord(float iX, float iY, float& pX, float& pY, float& pZ)
    {
        if (iX < 0 || iY < 0 || iX >= width - 1 || iY >= height - 1) {
            pX = pY = pZ = 0.f;
            return;
        }

        int x0 = int(iX);
        int y0 = int(iY);
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        float dx = iX - x0;
        float dy = iY - y0;

        auto D = [&](int x, int y) -> float {
            return float(depth[y * width + x]);
        };

        auto L = [&](int x, int y) -> FVector2f {
            int idx = (y * width + x) * 2;
            return FVector2f{
                lookup2D[idx],
                lookup2D[idx + 1]
            };
        };

        float d00 = D(x0, y0);
        float d10 = D(x1, y0);
        float d01 = D(x0, y1);
        float d11 = D(x1, y1);

        float d0 = d00 * (1 - dx) + d10 * dx;
        float d1 = d01 * (1 - dx) + d11 * dx;
        float d = d0 * (1 - dy) + d1 * dy;

        pZ = d * 0.1f;

        FVector2f r00 = L(x0, y0);
        FVector2f r10 = L(x1, y0);
        FVector2f r01 = L(x0, y1);
        FVector2f r11 = L(x1, y1);

        FVector2f r0{
            r00.X * (1 - dx) + r10.X * dx,
            r00.Y * (1 - dx) + r10.Y * dx
        };

        FVector2f r1{
            r01.X * (1 - dx) + r11.X * dx,
            r01.Y * (1 - dx) + r11.Y * dx
        };

        FVector2f r{
            r0.X * (1 - dy) + r1.X * dy,
            r0.Y * (1 - dy) + r1.Y * dy
        };

        pX = r.X * pZ;
        pY = r.Y * pZ;
    }
};