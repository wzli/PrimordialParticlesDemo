#pragma once

#include <cmath>
#include <vector>

class Particle {
    float x;
    float y;
    float theta;
}

class Particles {
    struct Config {
        float neighbour_radius = 5.0f;
        float particle_speed = 0.67f;
        float alpha = M_PI;
        float beta = 17 * M_PI/180;

        float simulation_radius = 100;
        size_t particle_population;
    };
    Particles(Config config) : _config(std::move(config)) {}

    Config _config;
    std::vector<Particle> _particles;
}
