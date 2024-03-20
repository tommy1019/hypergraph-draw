#include <float.h>
#include <stdio.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <type_traits>

#include <nlohmann/json.hpp>

#include "Vec2f.h"

#define JSON_ERR(msg, ...)                                                                                                                                                                                                 \
    do {                                                                                                                                                                                                                   \
        fprintf(stderr, msg "\n" __VA_OPT__(, ) __VA_ARGS__);                                                                                                                                                              \
        exit(1);                                                                                                                                                                                                           \
    } while (false);

struct Vertex {
    Vec2f pos;
    nlohmann::json json;
};

struct Hyperedge {
    std::vector<size_t> vertices;
    nlohmann::json json;
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

    if (!json.contains("vertices"))
        JSON_ERR("missing vertices field");

    if (!json.contains("edges"))
        JSON_ERR("missing edges field");

    struct {
        Vec2f min = { INFINITY, INFINITY };
        Vec2f max = { -INFINITY, -INFINITY };
        Vec2f size;
    } bounds;

    std::vector<Vertex> vertices;
    std::vector<Hyperedge> edges;

    {
        auto verts_json = json["vertices"];
        for (auto v : verts_json) {
            if (!v.contains("pos"))
                JSON_ERR("vertex missing position");

            auto pos_json = v["pos"];

            if (pos_json.size() != 2 || !pos_json[0].is_number() || !pos_json[1].is_number())
                JSON_ERR("vertex has invalid position data");

            Vertex res;
            res.pos.x = pos_json[0].get<double>();
            res.pos.y = pos_json[1].get<double>();
            res.json = v;

            if (res.pos.x < bounds.min.x)
                bounds.min.x = res.pos.x;
            if (res.pos.x > bounds.max.x)
                bounds.max.x = res.pos.x;
            if (res.pos.y < bounds.min.y)
                bounds.min.y = res.pos.y;
            if (res.pos.y > bounds.max.y)
                bounds.max.y = res.pos.y;

            vertices.push_back(res);
        }
    }

    {
        auto edges_json = json["edges"];
        for (auto e : edges_json) {
            if (!e.contains("vertices"))
                JSON_ERR("edge missing list of vertices");

            auto verts_json = e["vertices"];
            if (!verts_json.is_array())
                JSON_ERR("edges[].vertices must be an array");

            Hyperedge res;
            res.json = e;

            for (auto v : verts_json) {
                if (!v.is_number_unsigned())
                    JSON_ERR("edges[].vertices[] must be an index into the list of vertices");
                auto v_index = v.get<size_t>();
                if (v_index < 0 || v_index >= vertices.size())
                    JSON_ERR("edges[].vertices[] must be an index into the list of vertices");
                res.vertices.push_back(v_index);
            }

            edges.push_back(res);
        }
    }

    struct {
        double top = 30;
        double bottom = 30;
        double left = 30;
        double right = 30;
    } padding;

    if (json.contains("padding-top") && json["padding-top"].is_number())
        padding.top = json["padding-top"].get<double>();
    if (json.contains("padding-bottom") && json["padding-bottom"].is_number())
        padding.bottom = json["padding-bottom"].get<double>();
    if (json.contains("padding-left") && json["padding-left"].is_number())
        padding.left = json["padding-left"].get<double>();
    if (json.contains("padding-right") && json["padding-right"].is_number())
        padding.right = json["padding-right"].get<double>();

    double vertex_radius = 12;
    double edge_draw_radius = vertex_radius * 1.5;

    if (json.contains("vertex-radius") && json["vertex-radius"].is_number())
        vertex_radius = json["vertex-radius"].get<double>();

    if (json.contains("edge-draw-radius") && json["edge-draw-radius"].is_number())
        edge_draw_radius = json["edge-draw-radius"].get<double>();

    std::string vertex_fill = "black";
    double vertex_fill_opacity = 1;
    std::string vertex_stroke = "black";
    double vertex_stroke_opacity = 1;
    double vertex_stroke_width = 1;

    std::string edge_fill = "transparent";
    double edge_fill_opacity = 0.0;
    std::string edge_stroke = "black";
    double edge_stroke_opacity = 1;
    double edge_stroke_width = 1;
    bool edge_hull = false;

    {
        if (json.contains("vertex-fill") && json["vertex-fill"].is_string())
            vertex_fill = json["vertex-fill"].get<std::string>();

        if (json.contains("vertex-fill-opacity") && json["vertex-fill-opacity"].is_number())
            vertex_fill_opacity = json["vertex-fill-opacity"].get<double>();

        if (json.contains("vertex-stroke") && json["vertex-stroke"].is_string())
            vertex_stroke = json["vertex-stroke"].get<std::string>();

        if (json.contains("vertex-stroke-opacity") && json["vertex-stroke-opacity"].is_number())
            vertex_stroke_opacity = json["vertex-stroke-opacity"].get<double>();

        if (json.contains("vertex-stroke-width") && json["vertex-stroke-width"].is_number())
            vertex_stroke_width = json["vertex-stroke-width"].get<double>();
    }

    {
        if (json.contains("edge-fill") && json["edge-fill"].is_string())
            edge_fill = json["edge-fill"].get<std::string>();

        if (json.contains("edge-fill-opacity") && json["edge-fill-opacity"].is_number())
            edge_fill_opacity = json["edge-fill-opacity"].get<double>();

        if (json.contains("edge-stroke") && json["edge-stroke"].is_string())
            edge_stroke = json["edge-stroke"].get<std::string>();

        if (json.contains("edge-stroke-opacity") && json["edge-stroke-opacity"].is_number())
            edge_stroke_opacity = json["edge-stroke-opacity"].get<double>();

        if (json.contains("edge-stroke-width") && json["edge-stroke-width"].is_number())
            edge_stroke_width = json["edge-stroke-width"].get<double>();

        if (json.contains("edge-convex-hull") && json["edge-convex-hull"].is_boolean())
            edge_hull = json["edge-convex-hull"].get<bool>();
    }

    bounds.size = bounds.max - bounds.min;

    printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    printf("<svg version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"%f %f %f %f\">\n",
           bounds.min.x - padding.left,
           bounds.min.y - padding.top,
           bounds.size.x + padding.left + padding.right,
           bounds.size.y + padding.top + padding.bottom);

    for (auto e : edges) {

        std::string cur_edge_fill = edge_fill;
        double cur_edge_fill_opacity = edge_fill_opacity;
        std::string cur_edge_stroke = edge_stroke;
        double cur_edge_stroke_opacity = edge_stroke_opacity;
        double cur_edge_stroke_width = edge_stroke_width;

        double cur_edge_draw_radius = edge_draw_radius;

        bool cur_edge_hull = edge_hull;

        if (e.json.contains("fill") && e.json["fill"].is_string())
            cur_edge_fill = e.json["fill"].get<std::string>();

        if (e.json.contains("fill-opacity") && e.json["fill-opacity"].is_number())
            cur_edge_fill_opacity = e.json["fill-opacity"].get<double>();

        if (e.json.contains("stroke") && e.json["stroke"].is_string())
            cur_edge_stroke = e.json["stroke"].get<std::string>();

        if (e.json.contains("stroke-opacity") && e.json["stroke-opacity"].is_number())
            cur_edge_stroke_opacity = e.json["stroke-opacity"].get<double>();

        if (e.json.contains("stroke-width") && e.json["stroke-width"].is_number())
            cur_edge_stroke_width = e.json["stroke-width"].get<double>();

        if (e.json.contains("radius") && e.json["radius"].is_number())
            cur_edge_draw_radius = e.json["radius"].get<double>();

        if (e.json.contains("convex-hull") && e.json["convex-hull"].is_boolean())
            cur_edge_hull = e.json["convex-hull"].get<bool>();

        auto edge_verts = e.vertices;

        if (cur_edge_hull && edge_verts.size() > 3) {
            size_t min_index = 0;
            for (size_t i = 1; i < edge_verts.size(); i++) {
                if (vertices[edge_verts[i]].pos.x < vertices[edge_verts[min_index]].pos.x)
                    min_index = i;
            }

            enum Orientation { COLINEAR, CLOCKWISE, COUNTERCLOCKWISE };

            auto orientation = [](Vec2f p, Vec2f q, Vec2f r) -> Orientation {
                int val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

                if (val == 0)
                    return COLINEAR;
                return (val > 0) ? CLOCKWISE : COUNTERCLOCKWISE;
            };

            std::vector<size_t> new_verts = {};

            size_t cur_index = min_index;
            do {
                new_verts.push_back(cur_index);

                size_t best = (cur_index + 1) % edge_verts.size();

                for (int i = 0; i < edge_verts.size(); i++) {
                    if (orientation(vertices[edge_verts[cur_index]].pos, vertices[edge_verts[i]].pos, vertices[edge_verts[best]].pos) == COUNTERCLOCKWISE)
                        best = edge_verts[i];
                }

                cur_index = best;
            } while (cur_index != min_index);

            edge_verts = new_verts;
        }

        Vec2f mean = [&]() {
            Vec2f sum = {};
            for (auto v : e.vertices)
                sum += vertices[v].pos;
            return sum / (double)e.vertices.size();
        }();

        // Sort the vertices in clockwise order
        std::sort(edge_verts.begin(), edge_verts.end(), [&](size_t a, size_t b) {
            auto a_pos = vertices[a].pos - mean;
            auto b_pos = vertices[b].pos - mean;

            auto a_ang = std::atan2(a_pos.y, a_pos.x);
            auto b_ang = std::atan2(b_pos.y, b_pos.x);

            return a_ang < b_ang;
        });

        auto edge_line = [&](Vec2f a, Vec2f b, Vec2f c, std::string stroke, std::string fill, bool first) {
            auto offset_p1 = (a - b).rot90().normalized() * cur_edge_draw_radius;
            auto offset_p2 = (b - c).rot90().normalized() * cur_edge_draw_radius;

            auto p1 = b + offset_p1;
            auto p2 = b + offset_p2;

            auto o1 = a + offset_p1;
            auto o2 = c + offset_p2;

            auto a1 = std::atan2(offset_p1.y, offset_p1.x);
            auto a2 = std::atan2(offset_p2.y, offset_p2.x);

            auto ang = a1 - a2;

            if (1.0001 * ang > M_PI)
                ang -= 2 * M_PI;
            else if (0.99999 * ang < -M_PI)
                ang += 2 * M_PI;

            if (ang > 0) {
                double a1 = o1.y - p1.y;
                double b1 = p1.x - o1.x;
                double c1 = a1 * (p1.x) + b1 * (p1.y);

                double a2 = o2.y - p2.y;
                double b2 = p2.x - o2.x;
                double c2 = a2 * (p2.x) + b2 * (p2.y);

                auto determinant = a1 * b2 - a2 * b1;

                Vec2f intersect{ (b2 * c1 - b1 * c2) / determinant, (a1 * c2 - a2 * c1) / determinant };

                if (determinant < 0.0001) {
                    intersect = (p1 + p2) / 2.0f;
                }

                auto x1 = p1 + (intersect - p1) * 2;
                auto x2 = p2 + (intersect - p2) * 2;

                if (first) {
                    printf("        M %f %f\n", x2.x, x2.y);
                } else {
                    printf("        L %f %f\n", x1.x, x1.y);
                    printf("        A %f %f 0 %d 0 %f %f\n", cur_edge_draw_radius, cur_edge_draw_radius, 0, x2.x, x2.y);
                }

            } else {

                auto p1_ang = std::atan2((p1 - b).y, (p1 - b).x);
                auto p2_ang = std::atan2((p2 - b).y, (p2 - b).x);

                int large_arc = p2_ang - p1_ang < M_PI ? 0 : 1;

                if (first) {
                    printf("        M %f %f\n", p2.x, p2.y);
                } else {
                    printf("        L %f %f\n", p1.x, p1.y);
                    printf("        A %f %f 0 %d 1 %f %f\n", cur_edge_draw_radius, cur_edge_draw_radius, large_arc, p2.x, p2.y);
                }
            }
        };

        if (edge_verts.size() == 1) {
            printf("    <circle r=\"%f\" cx=\"%f\" cy=\"%f\" fill=\"%s\" fill-opacity=\"%f\" stroke=\"%s\" stroke-opacity=\"%f\" stroke-width=\"%f\" stroke-linecap=\"round\" />\n",
                   cur_edge_draw_radius,
                   vertices[edge_verts[0]].pos.x,
                   vertices[edge_verts[0]].pos.y,
                   cur_edge_fill.c_str(),
                   cur_edge_fill_opacity,
                   cur_edge_stroke.c_str(),
                   cur_edge_stroke_opacity,
                   cur_edge_stroke_width);
        } else if (edge_verts.size() == 2) {
            printf("    <path d=\"\n");

            edge_line(vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, vertices[edge_verts[0]].pos, edge_stroke, edge_fill, true);
            edge_line(vertices[edge_verts[1]].pos, vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, edge_stroke, edge_fill, false);
            edge_line(vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, vertices[edge_verts[0]].pos, edge_stroke, edge_fill, false);

            printf("        \" fill=\"%s\" fill-opacity=\"%f\" stroke=\"%s\" stroke-opacity=\"%f\" stroke-width=\"%f\" stroke-linecap=\"round\" />\n",
                   cur_edge_fill.c_str(),
                   cur_edge_fill_opacity,
                   cur_edge_stroke.c_str(),
                   cur_edge_stroke_opacity,
                   cur_edge_stroke_width);

        } else if (edge_verts.size() >= 2) {
            printf("    <path d=\"\n");

            auto prevprev = vertices[edge_verts[0]].pos;
            auto prev = vertices[edge_verts[1]].pos;

            edge_line(prevprev, prev, vertices[edge_verts[2]].pos, edge_stroke, edge_fill, true);

            prevprev = prev;
            prev = vertices[edge_verts[2]].pos;

            for (auto i = 3; i < edge_verts.size(); i++) {
                auto cur = vertices[edge_verts[i]].pos;

                edge_line(prevprev, prev, cur, edge_stroke, edge_fill, false);
                prevprev = prev;
                prev = cur;
            }
            edge_line(prevprev, prev, vertices[edge_verts[0]].pos, edge_stroke, edge_fill, false);
            edge_line(prev, vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, edge_stroke, edge_fill, false);
            edge_line(vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, vertices[edge_verts[2]].pos, edge_stroke, edge_fill, false);

            printf("        \" fill=\"%s\" fill-opacity=\"%f\" stroke=\"%s\" stroke-opacity=\"%f\" stroke-width=\"%f\" stroke-linecap=\"round\" />\n",
                   cur_edge_fill.c_str(),
                   cur_edge_fill_opacity,
                   cur_edge_stroke.c_str(),
                   cur_edge_stroke_opacity,
                   cur_edge_stroke_width);
        }

        if (e.json.contains("label") && e.json["label"].is_string()) {
            printf("<text x=\"%f\" y=\"%f\" dominant-baseline=\"middle\" text-anchor=\"middle\" font-size=\"10\">%s</text>", mean.x, mean.y, e.json["label"].get<std::string>().c_str());
        }
    }

    for (auto v : vertices) {
        std::string cur_vertex_fill = vertex_fill;
        double cur_vertex_fill_opacity = vertex_fill_opacity;
        std::string cur_vertex_stroke = vertex_stroke;
        double cur_vertex_stroke_opacity = vertex_stroke_opacity;
        double cur_vertex_stroke_width = vertex_stroke_width;

        double cur_vertex_radius = vertex_radius;

        if (v.json.contains("radius") && v.json["radius"].is_number())
            cur_vertex_radius = v.json["radius"].get<double>();

        if (v.json.contains("fill") && v.json["fill"].is_string())
            cur_vertex_fill = v.json["fill"].get<std::string>();

        if (v.json.contains("fill-opacity") && v.json["fill-opacity"].is_number())
            cur_vertex_fill_opacity = v.json["fill-opacity"].get<double>();

        if (v.json.contains("stroke") && v.json["stroke"].is_string())
            cur_vertex_stroke = v.json["stroke"].get<std::string>();

        if (v.json.contains("stroke-opacity") && v.json["stroke-opacity"].is_number())
            cur_vertex_stroke_opacity = v.json["stroke-opacity"].get<double>();

        if (v.json.contains("stroke-width") && v.json["stroke-width"].is_number())
            cur_vertex_stroke_width = v.json["stroke-width"].get<double>();

        printf("    <circle r=\"%f\" cx=\"%f\" cy=\"%f\" fill=\"%s\" fill-opacity=\"%f\" stroke=\"%s\" stroke-opacity=\"%f\" stroke-width=\"%f\" stroke-linecap=\"round\" />\n",
               cur_vertex_radius,
               v.pos.x,
               v.pos.y,
               cur_vertex_fill.c_str(),
               cur_vertex_fill_opacity,
               cur_vertex_stroke.c_str(),
               cur_vertex_stroke_opacity,
               cur_vertex_stroke_width);

        if (v.json.contains("label") && v.json["label"].is_string()) {
            printf("<text x=\"%f\" y=\"%f\" dominant-baseline=\"middle\" text-anchor=\"middle\" font-size=\"10\">%s</text>", v.pos.x, v.pos.y, v.json["label"].get<std::string>().c_str());
        }
    }

    printf("</svg>\n");
}
