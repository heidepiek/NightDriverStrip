#ifndef PatternBoxes_H
#define PatternBoxes_H

#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ArduinoJson.h>

#include "ledstripeffect.h"
#include "gfxbase.h"
#include "effects.h"

// Gebruik hetzelfde ID als in effects.h, bijvoorbeeld:
class PatternBoxes : public EffectWithId<idMatrixRandomBoxes> {
private:
    static constexpr int BOX_SIZE = 8;
    int _cols = 0;
    int _rows = 0;

    uint32_t _lastChangeMs = 0;
    int _changeIntervalMs = 200; // ms between updates

    int _colorBuckets = 8;
    std::vector<CRGB> _allowedColors;
    std::vector<CRGB> _boxColors;

    CRGBPalette16 _cachedPalette;

    const CRGBPalette16& CurrentPalette() const { return g()->GetCurrentPalette(); }

    CRGB NearestAllowed(const CRGB &src) const
    {
        if (_allowedColors.empty()) return src;
        uint32_t bestDist = UINT32_MAX;
        CRGB best = _allowedColors.front();
        for (const auto &c : _allowedColors)
        {
            int dr = int(src.r) - int(c.r);
            int dg = int(src.g) - int(c.g);
            int db = int(src.b) - int(c.b);
            uint32_t dist = uint32_t(dr*dr + dg*dg + db*db);
            if (dist < bestDist) { bestDist = dist; best = c; }
        }
        return best;
    }

    void RecomputeGrid()
    {
        _cols = MATRIX_WIDTH / BOX_SIZE;
        _rows = MATRIX_HEIGHT / BOX_SIZE;

        _allowedColors.clear();
        int buckets = std::max(1, _colorBuckets);
        _allowedColors.reserve(buckets);
        for (int b = 0; b < buckets; ++b)
        {
            uint8_t hue = static_cast<uint8_t>((b * 256) / buckets);
            _allowedColors.push_back(ColorFromPalette(CurrentPalette(), hue, 255, NOBLEND));
        }

        _cachedPalette = CurrentPalette();

        const int total = _cols * _rows;
        _boxColors.resize(total, CRGB::Black);

        for (int i = 0; i < total; ++i)
            _boxColors[i] = NearestAllowed(_boxColors[i]);
    }

    CRGB PickNonMatching(int idx)
    {
        if (_allowedColors.empty()) return RandomRainbowColor();

        int row = idx / _cols;
        int col = idx % _cols;

        CRGB neighbors[4];
        int ncount = 0;
        if (col > 0) neighbors[ncount++] = _boxColors[row * _cols + (col - 1)];
        if (col < _cols - 1) neighbors[ncount++] = _boxColors[row * _cols + (col + 1)];
        if (row > 0) neighbors[ncount++] = _boxColors[(row - 1) * _cols + col];
        if (row < _rows - 1) neighbors[ncount++] = _boxColors[(row + 1) * _cols + col];

        for (size_t attempt = 0; attempt < _allowedColors.size(); ++attempt)
        {
            const CRGB &cand = _allowedColors[random(_allowedColors.size())];
            bool conflict = false;
            for (int i = 0; i < ncount; ++i) if (cand == neighbors[i]) { conflict = true; break; }
            if (!conflict) return cand;
        }
        return _allowedColors[random(_allowedColors.size())];
    }

public:
    PatternBoxes() : EffectWithId<idMatrixRandomBoxes>("Boxes") {}

    PatternBoxes(const JsonObjectConst& obj) : EffectWithId<idMatrixRandomBoxes>(obj) {}

    void Start() override
    {
        _lastChangeMs = millis();
        RecomputeGrid();
    }

    void NextPalette()
    {
        g()->CyclePalette(1);
        RecomputeGrid();
    }

    void PrevPalette()
    {
        g()->CyclePalette(-1);
        RecomputeGrid();
    }

    bool SerializeSettingsToJSON(JsonObject& jsonObject) override
    {
        auto doc = CreateJsonDocument();
        JsonObject root = doc.to<JsonObject>();
        LEDStripEffect::SerializeSettingsToJSON(root);
        return SetIfNotOverflowed(doc, jsonObject, __PRETTY_FUNCTION__);
    }

    bool SetSetting(const String& name, const String& value) override
    {
        if (name == PTY_SIZE) return true; // size fixed
        return LEDStripEffect::SetSetting(name, value);
    }

    void Draw() override
    {
        if (_cols == 0 || _rows == 0) return;

        if (memcmp(&_cachedPalette, &CurrentPalette(), sizeof(CRGBPalette16)) != 0)
            RecomputeGrid();

        auto graphics = g();
        graphics->DimAll(240);

        uint32_t now = millis();
        if (now - _lastChangeMs >= static_cast<uint32_t>(_changeIntervalMs))
        {
            _lastChangeMs += (_changeIntervalMs * ((now - _lastChangeMs) / _changeIntervalMs));
            int changes = std::max(1, (_cols * _rows) / 6);
            const int total = _cols * _rows;
            for (int i = 0; i < changes; ++i)
            {
                int idx = random(total);
                _boxColors[idx] = PickNonMatching(idx);
            }
        }

        for (int r = 0; r < _rows; ++r)
        {
            for (int c = 0; c < _cols; ++c)
            {
                int idx = r * _cols + c;
                int x0 = c * BOX_SIZE;
                int y0 = r * BOX_SIZE;
                int x1 = std::min(x0 + BOX_SIZE, MATRIX_WIDTH);
                int y1 = std::min(y0 + BOX_SIZE, MATRIX_HEIGHT);
                graphics->fillRectangle(x0, y0, x1, y1, _boxColors[idx]);
            }
        }
    }
};

#endif // PatternBoxes_H
