#include <display.hpp>
#include <particles.hpp>
#include <zmq_http_server.hpp>
#include <vsm/zmq_transport.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <unistd.h>
#include <iostream>

static constexpr int UPDATE_INTERVAL = 20;  // ms

static constexpr char INDEX_RESPONSE[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
#include <index.html>
        ;

static constexpr char SVG_RESPONSE_HEADER[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/svg+xml\r\n"
        "Content-Encoding: gzip\r\n"
        "\r\n";

static constexpr char HELP_STRING[] =
        "Usage: sim_node [-i peer0,peer1,...] [-p http_port] [-b mesh_port] public_address";

static std::stringstream compress(std::stringstream& in) {
    namespace bio = boost::iostreams;
    std::stringstream ss;
    bio::filtering_streambuf<bio::input> out;
    out.push(bio::gzip_compressor(bio::gzip_params(bio::gzip::best_compression)));
    out.push(in);
    bio::copy(out, ss);
    return ss;
}

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

    vsm::MeshNode::Config mesh_config{
            vsm::msecs(1),  // peer update interval
            {
                    "self_name",              // name
                    "udp://127.0.0.1:11511",  // address
                    {100, 100},               // coordinates
                    4,                        // connection_degree
                    200,                      // lookup size
                    0,                        // rank decay
            },
            std::make_shared<vsm::ZmqTransport>("udp://*:11511"),  // transport
            std::make_shared<vsm::Logger>(),                       // logger
    };
    vsm::MeshNode mesh_node(mesh_config);

    Display display;
    Particles::Config sim_config;
    Particles particles(sim_config);
    ZmqHttpServer http_server(http_port);
    http_server.addTimer(UPDATE_INTERVAL, [&particles](int) { particles.update(); });

    http_server.addRequestHandler("/", [](zmq::message_t) {
        return zmq::message_t(
                const_cast<char*>(INDEX_RESPONSE), sizeof(INDEX_RESPONSE), nullptr, nullptr);
    });

    http_server.addRequestHandler("/spawn", [&particles](zmq::message_t msg) {
        float x, y;
        if (sscanf(static_cast<const char*>(msg.data()), "GET /spawn?x=%f&y=%f HTTP", &x, &y) ==
                2) {
            Particles::Point position(x, y);
            boost::geometry::subtract_value(position, 0.5f);
            boost::geometry::multiply_value(
                    position, particles.getConfig().simulation_radius * M_SQRT2);
            boost::geometry::add_point(position, particles.getConfig().simulation_origin);
            particles.spawnParticle(position);
        }
        static constexpr char response_data[] = "HTTP/1.1 204 No Content\r\n";
        return zmq::message_t(
                const_cast<char*>(response_data), sizeof(response_data), nullptr, nullptr);
    });

    http_server.addRequestHandler("/particles", [&particles, &display](zmq::message_t) {
        std::stringstream svg_stream;
        display.drawParticlesSvg(svg_stream, particles);
        auto response = SVG_RESPONSE_HEADER + compress(svg_stream).str();
        return zmq::message_t(response.c_str(), response.size());
    });

    http_server.addRequestHandler("/network", [&mesh_node, &display](zmq::message_t) {
        std::stringstream svg_stream;
        display.drawNetworkSvg(svg_stream, mesh_node);
        auto response = SVG_RESPONSE_HEADER + compress(svg_stream).str();
        return zmq::message_t(response.c_str(), response.size());
    });

    while (1) {
        try {
            http_server.poll();
        } catch (const zmq::error_t& e) {
            puts(e.what());
        }
    }
}
