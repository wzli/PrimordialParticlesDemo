#include <display.hpp>
#include <particles.hpp>
#include <zmq_http_server.hpp>
#include <vsm/zmq_transport.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <unistd.h>
#include <iostream>
#include <thread>

static constexpr int UPDATE_INTERVAL = 20;  // ms

static vsm::MeshNode::Config mesh_config{
        vsm::msecs(500),  // peer update interval
        {
                "node_name",              // name
                "udp://127.0.0.1:11511",  // address
                {0, 0},                   // coordinates
                10,                       // connection_degree
                200,                      // lookup size
                0,                        // rank decay
        },
        std::make_shared<vsm::ZmqTransport>("udp://*:11511"),  // transport
        std::make_shared<vsm::Logger>(),                       // logger
};

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
    // parse arguments
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
            case '?':
                puts(HELP_STRING);
                std::cerr << "Unknown option: '" << static_cast<char>(optopt) << "'" << std::endl;
                return -1;
            case ':':
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

    // particle sim objects
    Display display;
    Particles::Config sim_config;
    Particles particles(sim_config);

    // network objects
    vsm::MeshNode mesh_node(mesh_config);
    ZmqHttpServer http_server(http_port);

    // particle sim update timer
    http_server.addTimer(UPDATE_INTERVAL, [&particles](int) { particles.update(); });

    // default page
    http_server.addRequestHandler("/", [](zmq::message_t) {
        return zmq::message_t(
                const_cast<char*>(INDEX_RESPONSE), sizeof(INDEX_RESPONSE), nullptr, nullptr);
    });

    // spawn particle on click
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

    // generate particle display
    http_server.addRequestHandler("/particles", [&particles, &display](zmq::message_t) {
        std::stringstream svg_stream;
        display.drawParticlesSvg(svg_stream, particles);
        auto response = SVG_RESPONSE_HEADER + compress(svg_stream).str();
        return zmq::message_t(response.c_str(), response.size());
    });

    // generate network display
    http_server.addRequestHandler("/network", [&mesh_node, &display](zmq::message_t) {
        std::stringstream svg_stream;
        display.drawNetworkSvg(svg_stream, mesh_node);
        auto response = SVG_RESPONSE_HEADER + compress(svg_stream).str();
        return zmq::message_t(response.c_str(), response.size());
    });

    // worker thread runs mesh network
    std::thread mesh_thread([&mesh_node]() {
        while (1) {
            mesh_node.getTransport().poll(vsm::msecs(-1));
        }
    });
    mesh_thread.detach();

    // main thread runs http server and particle sim
    while (1) {
        try {
            http_server.poll();
        } catch (const zmq::error_t& e) {
            puts(e.what());
        }
    }
}
