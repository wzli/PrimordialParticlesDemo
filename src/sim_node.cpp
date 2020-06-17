#include <zmq_http_server.hpp>
#include <particles.hpp>
#include <unistd.h>
#include <iostream>
#include <sstream>

static constexpr char INDEX_HTML[] =
#include <index.html>
        ;

static constexpr char DISPLAY_SVG[] =
#include <display.svg>
        ;

static constexpr char HELP_STRING[] =
        "Usage: sim_node [-i peer0,peer1,...] [-p http_port] [-b mesh_port] public_address";

int main(int argc, char* argv[]) {
    const char* http_port = "8000";
    const char* mesh_port = "11511";
    const char* initial_peers = nullptr;
    int opt;
    while ((opt = getopt(argc, argv, ":i:p:b:")) != -1) {
        switch (opt) {
            case 'i':
                initial_peers = optarg;
                break;
            case 'p':
                http_port = optarg;
                break;
            case 'b':
                mesh_port = optarg;
                break;
            case '?':  // unknown option...
                puts(HELP_STRING);
                std::cerr << "Unknown option: '" << static_cast<char>(optopt) << "'" << std::endl;
                return -1;
            case ':':  // unknown option...
                puts(HELP_STRING);
                std::cerr << "Option: '" << static_cast<char>(optopt) << "' requires argument"
                          << std::endl;
                return -1;
        }
    }
    if (argc <= optind) {
        puts(HELP_STRING);
        return -1;
    }
    printf("i %s p %s b %s a %s\n", initial_peers, http_port, mesh_port, argv[optind]);

    Particles::Config sim_config;
    Particles particles(sim_config);
    particles.update();

    std::stringstream svg_stream;
    svg_stream << "<g fill=\"green\">\r\n";
    for (const auto& particle : particles.getParticles()) {
        svg_stream << "<circle r=\"10\" cx=\"" << particle.second.position.x() << "\" cy=\""
                   << particle.second.position.y() << "\"/>\r\n";
    }
    svg_stream << "</g>\r\n";
    printf("size %d\n", particles.getParticles().size());
    // std::cout << svg_stream.str();

    ZmqHttpServer http_server(http_port);

    http_server.addRequestHandler("/", [](zmq::message_t) {
        std::string response_data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "\r\n";
        response_data += INDEX_HTML;
        zmq::message_t response(response_data.c_str(), response_data.size());
        return response;
    });

    http_server.addRequestHandler("/display", [&](zmq::message_t) {
        std::stringstream response_stream;
        response_stream << "HTTP/1.1 200 OK\r\n"
                           "Content-Type: image/svg+xml\r\n"
                           "\r\n";
        response_stream << DISPLAY_SVG;
        response_stream << svg_stream.str();
        response_stream << "</svg>\r\n";
        auto response_data = response_stream.str();
        zmq::message_t response(response_data.c_str(), response_data.size());
        return response;
    });

    while (1) {
        try {
            http_server.poll();
        } catch (const zmq::error_t& e) {
            puts(e.what());
        }
    }
}
