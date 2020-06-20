#pragma once
#include <vsm/mesh_node.hpp>
#include <particles.hpp>
#include <sstream>

struct Display {
    struct Config {
        size_t svg_width = 1000;
        size_t svg_height = 1000;
        std::string svg_bg_color = "black";
        float particle_radius = 0.3f;
    };

    Config config;

    void drawNetworkSvg(std::stringstream& ss, vsm::MeshNode& mesh_node) const;
    void drawParticlesSvg(std::stringstream& ss, const Particles& particles) const;
    void drawNodeSvg(std::stringstream& ss, const vsm::NodeInfoT& node,
            const vsm::Vec2* from = nullptr) const;
    void writeSvgStartTag(std::stringstream& ss, float x, float y, float r) const;
    static const char* assignParticleColor(const Particles::Particle& particle);
};
