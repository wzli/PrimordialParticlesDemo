#pragma once
#include <particles.hpp>
#include <sstream>

struct Display {
    size_t svg_width = 1000;
    size_t svg_height = 1000;
    std::string svg_bg_color = "black";
    float particle_radius = 0.1f;

    void drawParticlesSvg(std::stringstream& ss, const Particles& particles) {
        auto r = particles.getConfig().simulation_radius;
        auto origin = particles.getConfig().simulation_origin;
        ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
        ss << "style=\"background-color:" << svg_bg_color << "\" ";
        ss << "width=\"" << svg_width << "\" ";
        ss << "height=\"" << svg_height << "\" ";
        ss << "viewBox=\"" << origin.x() - r << " " << origin.y() - r << " " << 2 * r << " "
           << 2 * r << "\" >\r\n";
        ss << "<g>\r\n";
        for (const auto& particle : particles.getParticles()) {
            ss << "<circle ";
            ss << "r=\"" << particle_radius << "\" ";
            ss << "cx=\"" << particle.second.position.x() << "\" ";
            ss << "cy=\"" << particle.second.position.y() << "\" ";
            ss << "fill=\"green"
                  "\" />\r\n";
        }
        ss << "</g>\r\n";
        ss << "</svg>\r\n";
    }
};
