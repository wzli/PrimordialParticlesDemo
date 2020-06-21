#include <display.hpp>

void Display::writeSvgStartTag(std::stringstream& ss, float x, float y, float r) const {
    ss << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
    ss << "xmlns:xlink=\"http://www.w3.org/1999/xlink\" ";
    ss << "style=\"background-color:" << config.svg_bg_color << "\" ";
    ss << "width=\"" << config.svg_width << "\" ";
    ss << "height=\"" << config.svg_height << "\" ";
    ss << "viewBox=\"" << x - r << " " << y - r << " " << 2 * r << " " << 2 * r << "\" >\r\n";
}

void Display::drawNetworkSvg(std::stringstream& ss, vsm::MeshNode& mesh_node) const {
    const auto& self = mesh_node.getPeerTracker().getNodeInfo();
    if (self.coordinates.empty()) {
        return;
    }
    writeSvgStartTag(ss, self.coordinates[0], self.coordinates[1], 50);
    for (const auto& peer : mesh_node.getConnectedPeers()) {
        const auto& peers = mesh_node.getPeerTracker().getPeers();
        const auto& peer_node = peers.find(peer);
        if (peer_node != peers.end()) {
            drawNodeSvg(ss, peer_node->second.node_info, &self.coordinates);
        }
    }
    drawNodeSvg(ss, self);
    ss << "</svg>\r\n";
}

void Display::drawParticlesSvg(std::stringstream& ss, const Particles& particles) const {
    float r = particles.getConfig().simulation_radius * M_SQRT1_2;
    auto origin = particles.getConfig().simulation_origin;
    writeSvgStartTag(ss, origin.x(), origin.y(), r);
    ss << "<g>\r\n";
    for (const auto& particle : particles.getParticles()) {
        if (particle.second.position.x() < origin.x() - r ||
                particle.second.position.x() > origin.x() + r ||
                particle.second.position.y() < origin.y() - r ||
                particle.second.position.y() > origin.y() + r) {
            continue;
        }
        ss << "<circle ";
        ss << "r=\"" << config.particle_radius << "\" ";
        ss << "cx=\"" << particle.second.position.x() << "\" ";
        ss << "cy=\"" << particle.second.position.y() << "\" ";
        ss << "fill=\"" << assignParticleColor(particle.second) << "\" />\r\n";
    }
    ss << "</g>\r\n";
    ss << "</svg>\r\n";
}

void Display::drawNodeSvg(
        std::stringstream& ss, const vsm::NodeInfoT& node, const std::vector<float>* from) const {
    if (node.coordinates.empty()) {
        return;
    }

    if (from) {
        ss << "<line ";
        ss << "stroke=\"dimgray\" ";
        ss << "stroke-width=\".3\" ";
        ss << "x1=\"" << (*from)[0] << "\" ";
        ss << "y1=\"" << (*from)[1] << "\" ";
        ss << "x2=\"" << node.coordinates[0] << "\" ";
        ss << "y2=\"" << node.coordinates[1] << "\" />\r\n";
    }

    int addr_start = node.address.find("/") + 2;
    int addr_len = node.address.rfind(":") - addr_start;
    std::string link = node.address.substr(addr_start, addr_len);

    ss << "<a target=\"_top\" xlink:href=\"http://" << link << "\" >\r\n";
    ss << "<g transform=\"translate(";
    ss << node.coordinates[0] << ",";
    ss << node.coordinates[1] << ") ";
    ss << "scale(.5) \" >\r\n";

    ss << "<ellipse ";
    ss << "rx=\"20\" ry=\"12\" ";
    ss << "stroke=\"" << (from ? "darkslategray" : "maroon") << "\" />\r\n";

    ss << "<text text-anchor=\"middle\" fill=\"white\" ";
    ss << "font-family=\"Arial, sans-serif\" ";
    ss << "font-size=\"4\" >\r\n";
    ss << "<tspan x=\"0\" y=\"-5\">" << link << "</tspan>\r\n";
    ss << "<tspan x=\"0\" y=\"1\">(" << node.coordinates[0] << ", " << node.coordinates[1]
       << ")</tspan>\r\n";
    ss << "<tspan x=\"0\" y=\"7\">" << node.name << "</tspan>\r\n";
    ss << "</text>";
    ss << "</g>";
    ss << "</a>\r\n";
}

const char* Display::assignParticleColor(const Particles::Particle& particle) {
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
