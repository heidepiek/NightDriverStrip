#ifndef PatternWarp_H
#define PatternWarp_H

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <ArduinoJson.h>

#include "ledstripeffect.h"
#include "gfxbase.h"
#include "effects.h"

// Gebruik hetzelfde ID als in effects.h
class PatternWarp : public EffectWithId<idMatrixWarp> {
private:
    struct Star {
        float angle;   // radians
        float cosA;
        float sinA;
        float z;       // diepte
        float speed;   // hoe snel z afneemt
        uint8_t hue;   // basis kleur (voor trail)
    };

    std::vector<Star> _stars;
    int _starCount = 1200;
    float _minZ = 0.10f;
    float _maxZ = 8.0f;
    float _focal = 10.0f;

    Star makeStar() {
        Star s;
        s.angle = static_cast<float>(random(0, 360)) * (M_PI / 180.0f);
        s.cosA = std::cos(s.angle);
        s.sinA = std::sin(s.angle);
        float r = static_cast<float>(random(0, 1000)) / 1000.0f;
        s.z = _minZ + std::pow(r, 0.5f) * (_maxZ - _minZ);
        s.speed = 0.02f + static_cast<float>(random(0, 1000)) / 1000.0f * 0.18f;
        s.hue = 160; // blauw
        return s;
    }

    void drawPixelSafe(int x, int y, const CRGB &c) {
        if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
        g()->fillRectangle(x, y, x + 1, y + 1, c);
    }

    void drawLineSafe(int x0, int y0, int x1, int y1, const CRGB &c) {
        int dx = x1 - x0;
        int dy = y1 - y0;
        int steps = std::max(std::abs(dx), std::abs(dy));
        if (steps <= 0) { drawPixelSafe(x0, y0, c); return; }
        for (int i = 0; i <= steps; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(steps);
            int x = x0 + static_cast<int>(std::round(dx * t));
            int y = y0 + static_cast<int>(std::round(dy * t));
            drawPixelSafe(x, y, c);
        }
    }

public:
    PatternWarp() : EffectWithId<idMatrixWarp>("Warp") {}
    PatternWarp(const JsonObjectConst& obj) : EffectWithId<idMatrixWarp>(obj) {}

    void Start() override {
        _stars.clear();
        _stars.reserve(_starCount);
        for (int i = 0; i < _starCount; ++i) _stars.push_back(makeStar());
    }

    void Draw() override {
        g()->DimAll(180); // iets zachter dimmen voor langere trail

        const int cx = MATRIX_WIDTH / 2;
        const int cy = MATRIX_HEIGHT / 2;

        for (auto &s : _stars) {
            float prevZ = s.z;
            float prevScale = _focal / prevZ;
            float px0f = cx + s.cosA * prevScale;
            float py0f = cy + s.sinA * prevScale;

            s.z -= s.speed;
            if (s.z <= _minZ) {
                s = makeStar();
                prevZ = s.z + s.speed;
                prevScale = _focal / prevZ;
                px0f = cx + std::cos(s.angle) * prevScale;
                py0f = cy + std::sin(s.angle) * prevScale;
            }

            float curScale = _focal / s.z;
            float px1f = cx + s.cosA * curScale;
            float py1f = cy + s.sinA * curScale;

            int x0 = static_cast<int>(std::round(px0f));
            int y0 = static_cast<int>(std::round(py0f));
            int x1 = static_cast<int>(std::round(px1f));
            int y1 = static_cast<int>(std::round(py1f));

            // Warp effect: fel wit in het centrum, langere blauwe trail
            float t = (s.z - _minZ) / (_maxZ - _minZ);  // 0 = dichtbij, 1 = ver
            uint8_t trailBri = static_cast<uint8_t>(255 * (1.0f - t));
            CRGB trailColor = CHSV(s.hue, 255, trailBri);

            // Blend wit voor kern, groter effect naar centrum
            float centerFactor = std::pow(1.0f - t, 2.0f); // exponentieel voor fel wit in midden
            CRGB col = blend(CRGB::White, trailColor, static_cast<uint8_t>((1.0f - centerFactor) * 255));

            drawLineSafe(x0, y0, x1, y1, col);
        }
    }
};

#endif // PatternWarp_H
