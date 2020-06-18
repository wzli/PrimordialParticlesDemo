#pragma once
#include <particles.hpp>
#include <sstream>

struct Display {
    struct Config {
        size_t svg_width = 1000;
        size_t svg_height = 1000;
        std::string svg_bg_color = "black";
        float particle_radius = 0.2f;
    };

    Config config;

    void drawParticlesSvg(std::stringstream& ss, const Particles& particles) const;
    static const char* assignParticleColor(const Particles::Particle& particle);
};
