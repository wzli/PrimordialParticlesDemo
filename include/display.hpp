#pragma once
#include <particles.hpp>
#include <sstream>

struct Display {
    size_t svg_width = 1000;
    size_t svg_height = 1000;
    std::string svg_bg_color = "black";
    float particle_radius = 0.2f;

    static const char* assignParticleColor(const Particles::Particle& particle) {
        size_t total_neighbors = particle.left_neighbors + particle.right_neighbors;
        if (total_neighbors > 35) {
            return "yellow";
        } else if (total_neighbors > 16) {
            return "blue";
        } else if (particle.close_neighbors > 15) {
            return "magenta";
        } else if (total_neighbors > 13) {
            return "brown";
        } else {
            return "green";
        }
    }

    void drawParticlesSvg(std::stringstream& ss, const Particles& particles) const {
        auto r = particles.getConfig().simulation_radius * M_SQRT1_2;
        auto origin = particles.getConfig().simulation_origin;
        ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
        ss << "style=\"background-color:" << svg_bg_color << "\" ";
        ss << "width=\"" << svg_width << "\" ";
        ss << "height=\"" << svg_height << "\" ";
        ss << "viewBox=\"" << origin.x() - r << " " << origin.y() - r << " " << 2 * r << " "
           << 2 * r << "\" >\r\n";
        ss << "<g>\r\n";
        for (const auto& particle : particles.getParticles()) {
            if (particle.second.position.x() < origin.x() - r ||
                    particle.second.position.x() > origin.x() + r ||
                    particle.second.position.y() < origin.y() - r ||
                    particle.second.position.y() > origin.y() + r) {
                continue;
            }
            ss << "<circle ";
            ss << "r=\"" << particle_radius << "\" ";
            ss << "cx=\"" << particle.second.position.x() << "\" ";
            ss << "cy=\"" << particle.second.position.y() << "\" ";
            ss << "fill=\"" << assignParticleColor(particle.second) << "\" />\r\n";
        }
        ss << "</g>\r\n";
        ss << "</svg>\r\n";
    }
};
