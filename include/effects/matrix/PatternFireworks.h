#ifndef PatternFireworks_H
#define PatternFireworks_H

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <ArduinoJson.h>
#include "ledstripeffect.h"
#include "gfxbase.h"
#include "effects.h"

class PatternFireworks : public EffectWithId<idMatrixFireworks> {
private:
    struct Particle {
        float x, y;
        float vx, vy;
        uint8_t hue;
        uint8_t life;
        bool exploded;
        bool spark;
    };

    std::vector<Particle> _particles;
    int _maxRockets = 5;
    int _maxParticles = 300;
    int _explosionSize = 30;

    Particle makeRocket() {
        Particle p;
        p.x = random(0, MATRIX_WIDTH);
        p.y = MATRIX_HEIGHT - 1;
        p.vx = (float(random(-20, 21)) / 100.0f);
        p.vy = -(0.5f + (float(random(0, 50)) / 100.0f));
        p.hue = random(0, 255);
        p.life = 0;
        p.exploded = false;
        p.spark = false;
        return p;
    }

    Particle makeSpark(float x, float y, uint8_t hue) {
        Particle p;
        p.x = x;
        p.y = y;
        float angle = (float)random(0, 360) * (M_PI / 180.0f);
        float speed = 0.3f + (float)random(0, 100) / 200.0f;
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed;
        p.hue = hue;
        p.life = 0;
        p.exploded = true;
        p.spark = true;
        return p;
    }

    void drawPixelSafe(int x, int y, const CRGB &c) {
        if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
        g()->fillRectangle(x, y, x + 1, y + 1, c);
    }

public:
    PatternFireworks() : EffectWithId<idMatrixFireworks>("Fireworks2") {}
    PatternFireworks(const JsonObjectConst& obj) : EffectWithId<idMatrixFireworks>(obj) {}

    void Start() override {
        _particles.clear();
        _particles.reserve(_maxParticles);
    }

    void Draw() override {
        g()->DimAll(200);

        if (_particles.size() < _maxParticles && random(0, 10) < 3) {
            int activeRockets = 0;
            for (auto &p : _particles)
                if (!p.exploded) activeRockets++;
            if (activeRockets < _maxRockets)
                _particles.push_back(makeRocket());
        }

        std::vector<Particle> newParticles;

        for (auto &p : _particles) {
            if (!p.exploded) {
                p.x += p.vx;
                p.y += p.vy;
                p.vy += 0.01f;
                drawPixelSafe((int)round(p.x), (int)round(p.y), CHSV(p.hue, 255, 255));

                if (p.vy >= 0 || random(0, 100) < 5) {
                    p.exploded = true;
                    for (int i = 0; i < _explosionSize; ++i) {
                        newParticles.push_back(makeSpark(p.x, p.y, (p.hue + random(0, 50)) % 255));
                    }
                }
            } else {
                p.x += p.vx;
                p.y += p.vy;
                p.vy += 0.02f;
                p.life++;
                uint8_t bri = (p.life < 50) ? 255 - (p.life * 5) : 0;

                if (bri > 0) {
                    CRGB col = CHSV(p.hue, 255, bri);
                    if (p.spark && random(0, 100) < 20) {
                        col = CHSV(p.hue, 0, bri);
                    }
                    drawPixelSafe((int)round(p.x), (int)round(p.y), col);
                }
            }
        }

        _particles.insert(_particles.end(), newParticles.begin(), newParticles.end());

        _particles.erase(
            std::remove_if(_particles.begin(), _particles.end(),
                [](const Particle &p){ return p.life > 50 || p.y >= MATRIX_HEIGHT; }),
            _particles.end());
    }
};

#endif // PatternFireworks_H
