#include <float.h>
#include <stdio.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <type_traits>

#include <gvc.h>
#include <nlohmann/json.hpp>

#include "Vec2f.h"

#define JSON_ERR(msg, ...)                                                                                                                                                                                                 \
    do {                                                                                                                                                                                                                   \
        fprintf(stderr, msg "\n" __VA_OPT__(, ) __VA_ARGS__);                                                                                                                                                              \
        exit(1);                                                                                                                                                                                                           \
    } while (false);

struct Vertex {
    size_t index;
    Agnode_t* node;
    nlohmann::json json;
};

struct Hyperedge {
    std::vector<size_t> vertices;
};

int main(int argc, char** argv) {

    nlohmann::json json;

    if (argc > 1) {
        auto file = std::ifstream(argv[1]);
        if (!file) {
            fprintf(stderr, "Could not open file '%s'\n", argv[1]);
            return 0;
        }
        file >> json;
    } else {
        std::cin >> json;
    }

    if (!json.contains("edges") || !json["edges"].is_array())
        JSON_ERR("missing edges field");

    std::vector<Vertex> vertices;
    std::vector<Hyperedge> edges;

    if (json.contains("vertices")) {
        if (json["vertices"].is_array()) {
            size_t index = 0;
            auto verts_json = json["vertices"];
            for (auto v : verts_json) {
                Vertex res;
                res.index = index++;
                res.json = v;
                vertices.push_back(res);
            }
        } else if (json["vertices"].is_object()) {

            for (auto& p : json["vertices"].items()) {
                auto index = [&]() {
                    try {
                        return std::stoi(p.key());
                    } catch (...) {
                        JSON_ERR("keys in vertices object must be indexes")
                    }
                }();

                while (vertices.size() < index + 1) {
                    vertices.push_back({ .index = vertices.size() - 1 });
                }

                vertices[index].json = p.value();
            }

        } else {
            JSON_ERR("invalid vertices field");
        }
    }

    {
        size_t max_vert_index = 0;

        auto edges_json = json["edges"];
        for (auto e : edges_json) {
            if (!e.contains("vertices"))
                JSON_ERR("edge missing list of vertices");

            auto verts_json = e["vertices"];
            if (!verts_json.is_array())
                JSON_ERR("edge[].vertices must be an array");

            Hyperedge res;

            for (auto v : verts_json) {
                if (!v.is_number_unsigned())
                    JSON_ERR("edge[].vertices[] must be an index into the list of vertices");
                auto v_index = v.get<size_t>();
                if (v_index < 0)
                    JSON_ERR("edge[].vertices[] must be an index into the list of vertices");
                max_vert_index = std::max(max_vert_index, v_index);
                res.vertices.push_back(v_index);
            }

            edges.push_back(res);
        }

        while (vertices.size() < max_vert_index + 1) {
            vertices.push_back({ .index = vertices.size() - 1 });
        }
    }

    auto gvc = gvContext();

    Agraph_t* g = agopen(0, Agundirected, 0);

    for (auto& v : vertices) {
        v.node = agnode(g, 0, 1);
    }

    for (auto e : edges) {
        // Create a complete graph with the edges

        auto rec = [&](auto rec, std::vector<size_t> remaining_verts) {
            if (remaining_verts.size() <= 1)
                return;

            auto back = remaining_verts.back();
            remaining_verts.pop_back();

            for (auto v : remaining_verts)
                agedge(g, vertices[back].node, vertices[v].node, 0, 1);

            rec(rec, remaining_verts);
        };

        rec(rec, e.vertices);
    }

    std::string layout = "neato";
    if (json.contains("layout-engine") && json["layout-engine"].is_string())
        layout = json["layout-engine"];

    gvLayout(gvc, g, layout.c_str());

    gvRender(gvc, g, "dot", 0);

    nlohmann::json verts_json = {};

    auto pos_str = strdup("pos");

    for (auto i = 0; i < vertices.size(); i++) {
        auto pos_string = std::string(agget(vertices[i].node, pos_str));
        double x = std::stof(pos_string.substr(0, pos_string.find_first_of(",")));
        double y = std::stof(pos_string.substr(pos_string.find_first_of(",") + 1));
        vertices[i].json["pos"] = nlohmann::json::array({ x, y });
        verts_json.push_back(vertices[i].json);
    }

    json["vertices"] = verts_json;

    std::cout << json;
}
