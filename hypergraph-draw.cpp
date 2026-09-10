#include <float.h>
#include <optional>
#include <stdio.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>
#include <vector>

#include "Vec2f.h"

#define JSON_ERR(msg, ...)                                                                                                                                                                             \
    do {                                                                                                                                                                                               \
        fprintf(stderr, msg "\n" __VA_OPT__(, ) __VA_ARGS__);                                                                                                                                          \
        exit(1);                                                                                                                                                                                       \
    } while (false);

struct Vertex {
    Vec2f pos;
    nlohmann::json json;
};

struct Hyperedge {
    std::vector<size_t> vertices;
    nlohmann::json json;
};

struct Color {
    float r, g, b;
};

struct SVGOutput {
    void begin(Vec2f min, Vec2f max) {
        printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
        printf("<svg version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"%f %f %f %f\">\n", min.x, min.y, max.x, max.y);
    }
    void end() { printf("</svg>\n"); }

    void circle(Vec2f pos, float r, Color fill, float fill_opacity, Color stroke, float stroke_opacity, float stroke_width, std::optional<std::string> label = {}) {
        printf("    <circle r=\"%f\" cx=\"%f\" cy=\"%f\" fill=\"rgba(%d, %d, %d, %f)\" stroke=\"rgba(%d, %d, %d, %f)\" stroke-width=\"%f\" stroke-linecap=\"round\" />\n",
               r,
               pos.x,
               pos.y,
               (int)std::floor(fill.r * 255),
               (int)std::floor(fill.g * 255),
               (int)std::floor(fill.b * 255),
               fill_opacity,
               (int)std::floor(stroke.r * 255),
               (int)std::floor(stroke.g * 255),
               (int)std::floor(stroke.b * 255),
               stroke_opacity,
               stroke_width);

        if (label.has_value()) {
            printf("<text x=\"%f\" y=\"%f\" dominant-baseline=\"middle\" text-anchor=\"middle\" font-size=\"10\">%s</text>", pos.x, pos.y, label.value().c_str());
        }
    }

    void path(Color fill, float fill_opacity, Color stroke, float stroke_opacity, float stroke_width, std::string dash_array, std::optional<std::string> label, auto draw) {
        printf("    <path d=\"\n");

        Vec2f mean = {};
        float count = 0;

        draw(
            [&](Vec2f p) {
                printf("        M %f %f\n", p.x, p.y);
                mean.x += p.x;
                mean.y += p.y;
                count++;
            },
            [&](Vec2f p) {
                printf("        L %f %f\n", p.x, p.y);
                mean.x += p.x;
                mean.y += p.y;
                count++;
            },
            [&](Vec2f p, Vec2f r, int large_arc, int sweep) {
                printf("        A %f %f 0 %d %d %f %f\n", r.x, r.y, large_arc, sweep, p.x, p.y);
                mean.x += p.x;
                mean.y += p.y;
                count++;
            });

        mean.x /= count;
        mean.y /= count;

        printf("        \" fill=\"rgba(%d, %d, %d, %f)\" stroke=\"rgba(%d, %d, %d, %f)\" stroke-width=\"%f\" stroke-linecap=\"round\" stroke-dasharray=\"%s\" />\n",
               (int)std::floor(fill.r * 255),
               (int)std::floor(fill.g * 255),
               (int)std::floor(fill.b * 255),
               fill_opacity,
               (int)std::floor(stroke.r * 255),
               (int)std::floor(stroke.g * 255),
               (int)std::floor(stroke.b * 255),
               stroke_opacity,
               stroke_width,
               dash_array.c_str());

        if (label.has_value())
            printf("<text x=\"%f\" y=\"%f\" dominant-baseline=\"middle\" text-anchor=\"middle\" font-size=\"10\">%s</text>", mean.x, mean.y, label.value().c_str());
    }
};

struct TIKZOutput {
    struct Label {
        std::string text;
        Vec2f pos;
    };

    std::vector<Label> labels;

    void begin(Vec2f min, Vec2f max) { printf("\\begin{tikzpicture}\n"); }
    void end() {

        for (auto& l : labels) {
            printf("    \\node at (%fpt, %fpt) {%s};\n", l.pos.x, l.pos.y, l.text.c_str());
        }

        printf("\\end{tikzpicture}\n");
    }

    void circle(Vec2f pos, float r, Color fill, float fill_opacity, Color stroke, float stroke_opacity, float stroke_width, std::optional<std::string> label = {}) {
        printf("    \\filldraw[draw={rgb,255:red,%d; green,%d; blue,%d}, draw opacity=%f, line width=%fpt, fill={rgb,255:red,%d; green,%d; blue,%d}, fill opacity=%f] (%fpt,%fpt) circle (%fpt);\n",
               (int)std::floor(stroke.r * 255),
               (int)std::floor(stroke.g * 255),
               (int)std::floor(stroke.b * 255),
               stroke_opacity,
               stroke_width,
               (int)std::floor(fill.r * 255),
               (int)std::floor(fill.g * 255),
               (int)std::floor(fill.b * 255),
               fill_opacity,
               pos.x,
               -pos.y,
               r);

        if (label.has_value()) {
            labels.push_back({.text = label.value(), .pos = pos});
        }
    }

    void path(Color fill, float fill_opacity, Color stroke, float stroke_opacity, float stroke_width, std::string dash_array, std::optional<std::string> label, auto draw) {

        printf(
            "    \\filldraw[draw={rgb,255:red,%d; green,%d; blue,%d}, draw opacity=%f, line width=%fpt, fill={rgb,255:red,%d; green,%d; blue,%d}, fill opacity=%f%s] svg \"\n",
            (int)std::floor(stroke.r * 255),
            (int)std::floor(stroke.g * 255),
            (int)std::floor(stroke.b * 255),
            stroke_opacity,
            stroke_width,
            (int)std::floor(fill.r * 255),
            (int)std::floor(fill.g * 255),
            (int)std::floor(fill.b * 255),
            fill_opacity,
            (dash_array == "none" ? std::string("") : [&]() {
                std::string new_arr = ", dash pattern={";

                std::istringstream ss(dash_array);

                bool on = true;
                bool first = true;

                int num;
                while (ss >> num) {

                    new_arr = new_arr + (first ? "" : " ") + (on ? "on" : "off") + " " + std::to_string(num) + "pt";

                    on = !on;
                    first = false;
                }

                return new_arr + "}";
            }()).c_str());

        Vec2f mean = {};
        float count = 0;

        draw(
            [&](Vec2f p) {
                printf("        M %f %f\n", p.x, -p.y);
                mean.x += p.x;
                mean.y += p.y;
                count++;
            },
            [&](Vec2f p) {
                printf("        L %f %f\n", p.x, -p.y);
                mean.x += p.x;
                mean.y += p.y;
                count++;
            },
            [&](Vec2f p, Vec2f r, int large_arc, int sweep) {
                printf("        A %f %f 0 %d %d %f %f\n", r.x, r.y * 1.0001, large_arc, sweep == 1 ? 0 : 1, p.x, -p.y);
                mean.x += p.x;
                mean.y += p.y;
                count++;
            });

        mean.x /= count;
        mean.y /= count;

        printf("    \";\n");

        if (label.has_value()) {
            labels.push_back({.text = label.value(), .pos = mean});
        }
    }
};

void draw(nlohmann::json json, auto& output) {
    if (!json.contains("vertices"))
        JSON_ERR("missing vertices field");

    if (!json.contains("edges"))
        JSON_ERR("missing edges field");

    struct {
        Vec2f min = {INFINITY, INFINITY};
        Vec2f max = {-INFINITY, -INFINITY};
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
            res.pos.x = pos_json[0].get<float>();
            res.pos.y = pos_json[1].get<float>();
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
        float top = 30;
        float bottom = 30;
        float left = 30;
        float right = 30;
    } padding;

    if (json.contains("padding-top") && json["padding-top"].is_number())
        padding.top = json["padding-top"].get<float>();
    if (json.contains("padding-bottom") && json["padding-bottom"].is_number())
        padding.bottom = json["padding-bottom"].get<float>();
    if (json.contains("padding-left") && json["padding-left"].is_number())
        padding.left = json["padding-left"].get<float>();
    if (json.contains("padding-right") && json["padding-right"].is_number())
        padding.right = json["padding-right"].get<float>();

    float vertex_radius = 12;
    float edge_draw_radius = vertex_radius * 1.5;

    if (json.contains("vertex-radius") && json["vertex-radius"].is_number())
        vertex_radius = json["vertex-radius"].get<float>();

    if (json.contains("edge-draw-radius") && json["edge-draw-radius"].is_number())
        edge_draw_radius = json["edge-draw-radius"].get<float>();

    auto name_to_color = [](std::string name) -> Color {
        if (name.starts_with("rgb(")) {
            std::stringstream ss(name.substr(4));

            Color res;

            std::string token;

            if (std::getline(ss, token, ',')) {
                res.r = std::stod(token) / 255.0;
            } else {
                JSON_ERR("Malformed rgb string: %s", name.c_str());
            }

            if (std::getline(ss, token, ',')) {
                res.g = std::stod(token) / 255.0;
            } else {
                JSON_ERR("Malformed rgb string: %s", name.c_str());
            }

            if (std::getline(ss, token, ',')) {
                res.b = std::stod(token) / 255.0;
            } else {
                JSON_ERR("Malformed rgb string: %s", name.c_str());
            }

            return res;
        }

        if (name == "transparent" || name == "clear")
            return Color{.r = 0, .g = 0, .b = 0};

        if (name == "aliceblue") {
            return Color{.r = 240.0 / 255.0, .g = 248.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "antiquewhite") {
            return Color{.r = 250.0 / 255.0, .g = 235.0 / 255.0, .b = 215.0 / 255.0};
        } else if (name == "aqua") {
            return Color{.r = 0.0 / 255.0, .g = 255.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "aquamarine") {
            return Color{.r = 127.0 / 255.0, .g = 255.0 / 255.0, .b = 212.0 / 255.0};
        } else if (name == "azure") {
            return Color{.r = 240.0 / 255.0, .g = 255.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "beige") {
            return Color{.r = 245.0 / 255.0, .g = 245.0 / 255.0, .b = 220.0 / 255.0};
        } else if (name == "bisque") {
            return Color{.r = 255.0 / 255.0, .g = 228.0 / 255.0, .b = 196.0 / 255.0};
        } else if (name == "black") {
            return Color{.r = 0.0 / 255.0, .g = 0.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "blanchedalmond") {
            return Color{.r = 255.0 / 255.0, .g = 235.0 / 255.0, .b = 205.0 / 255.0};
        } else if (name == "blue") {
            return Color{.r = 0.0 / 255.0, .g = 0.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "blueviolet") {
            return Color{.r = 138.0 / 255.0, .g = 43.0 / 255.0, .b = 226.0 / 255.0};
        } else if (name == "brown") {
            return Color{.r = 165.0 / 255.0, .g = 42.0 / 255.0, .b = 42.0 / 255.0};
        } else if (name == "burlywood") {
            return Color{.r = 222.0 / 255.0, .g = 184.0 / 255.0, .b = 135.0 / 255.0};
        } else if (name == "cadetblue") {
            return Color{.r = 95.0 / 255.0, .g = 158.0 / 255.0, .b = 160.0 / 255.0};
        } else if (name == "chartreuse") {
            return Color{.r = 127.0 / 255.0, .g = 255.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "chocolate") {
            return Color{.r = 210.0 / 255.0, .g = 105.0 / 255.0, .b = 30.0 / 255.0};
        } else if (name == "coral") {
            return Color{.r = 255.0 / 255.0, .g = 127.0 / 255.0, .b = 80.0 / 255.0};
        } else if (name == "cornflowerblue") {
            return Color{.r = 100.0 / 255.0, .g = 149.0 / 255.0, .b = 237.0 / 255.0};
        } else if (name == "cornsilk") {
            return Color{.r = 255.0 / 255.0, .g = 248.0 / 255.0, .b = 220.0 / 255.0};
        } else if (name == "crimson") {
            return Color{.r = 220.0 / 255.0, .g = 20.0 / 255.0, .b = 60.0 / 255.0};
        } else if (name == "cyan") {
            return Color{.r = 0.0 / 255.0, .g = 255.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "darkblue") {
            return Color{.r = 0.0 / 255.0, .g = 0.0 / 255.0, .b = 139.0 / 255.0};
        } else if (name == "darkcyan") {
            return Color{.r = 0.0 / 255.0, .g = 139.0 / 255.0, .b = 139.0 / 255.0};
        } else if (name == "darkgoldenrod") {
            return Color{.r = 184.0 / 255.0, .g = 134.0 / 255.0, .b = 11.0 / 255.0};
        } else if (name == "darkgray") {
            return Color{.r = 169.0 / 255.0, .g = 169.0 / 255.0, .b = 169.0 / 255.0};
        } else if (name == "darkgreen") {
            return Color{.r = 0.0 / 255.0, .g = 100.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "darkgrey") {
            return Color{.r = 169.0 / 255.0, .g = 169.0 / 255.0, .b = 169.0 / 255.0};
        } else if (name == "darkkhaki") {
            return Color{.r = 189.0 / 255.0, .g = 183.0 / 255.0, .b = 107.0 / 255.0};
        } else if (name == "darkmagenta") {
            return Color{.r = 139.0 / 255.0, .g = 0.0 / 255.0, .b = 139.0 / 255.0};
        } else if (name == "darkolivegreen") {
            return Color{.r = 85.0 / 255.0, .g = 107.0 / 255.0, .b = 47.0 / 255.0};
        } else if (name == "darkorange") {
            return Color{.r = 255.0 / 255.0, .g = 140.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "darkorchid") {
            return Color{.r = 153.0 / 255.0, .g = 50.0 / 255.0, .b = 204.0 / 255.0};
        } else if (name == "darkred") {
            return Color{.r = 139.0 / 255.0, .g = 0.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "darksalmon") {
            return Color{.r = 233.0 / 255.0, .g = 150.0 / 255.0, .b = 122.0 / 255.0};
        } else if (name == "darkseagreen") {
            return Color{.r = 143.0 / 255.0, .g = 188.0 / 255.0, .b = 143.0 / 255.0};
        } else if (name == "darkslateblue") {
            return Color{.r = 72.0 / 255.0, .g = 61.0 / 255.0, .b = 139.0 / 255.0};
        } else if (name == "darkslategray") {
            return Color{.r = 47.0 / 255.0, .g = 79.0 / 255.0, .b = 79.0 / 255.0};
        } else if (name == "darkslategrey") {
            return Color{.r = 47.0 / 255.0, .g = 79.0 / 255.0, .b = 79.0 / 255.0};
        } else if (name == "darkturquoise") {
            return Color{.r = 0.0 / 255.0, .g = 206.0 / 255.0, .b = 209.0 / 255.0};
        } else if (name == "darkviolet") {
            return Color{.r = 148.0 / 255.0, .g = 0.0 / 255.0, .b = 211.0 / 255.0};
        } else if (name == "deeppink") {
            return Color{.r = 255.0 / 255.0, .g = 20.0 / 255.0, .b = 147.0 / 255.0};
        } else if (name == "deepskyblue") {
            return Color{.r = 0.0 / 255.0, .g = 191.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "dimgray") {
            return Color{.r = 105.0 / 255.0, .g = 105.0 / 255.0, .b = 105.0 / 255.0};
        } else if (name == "dimgrey") {
            return Color{.r = 105.0 / 255.0, .g = 105.0 / 255.0, .b = 105.0 / 255.0};
        } else if (name == "dodgerblue") {
            return Color{.r = 30.0 / 255.0, .g = 144.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "firebrick") {
            return Color{.r = 178.0 / 255.0, .g = 34.0 / 255.0, .b = 34.0 / 255.0};
        } else if (name == "floralwhite") {
            return Color{.r = 255.0 / 255.0, .g = 250.0 / 255.0, .b = 240.0 / 255.0};
        } else if (name == "forestgreen") {
            return Color{.r = 34.0 / 255.0, .g = 139.0 / 255.0, .b = 34.0 / 255.0};
        } else if (name == "fuchsia") {
            return Color{.r = 255.0 / 255.0, .g = 0.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "gainsboro") {
            return Color{.r = 220.0 / 255.0, .g = 220.0 / 255.0, .b = 220.0 / 255.0};
        } else if (name == "ghostwhite") {
            return Color{.r = 248.0 / 255.0, .g = 248.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "gold") {
            return Color{.r = 255.0 / 255.0, .g = 215.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "goldenrod") {
            return Color{.r = 218.0 / 255.0, .g = 165.0 / 255.0, .b = 32.0 / 255.0};
        } else if (name == "gray") {
            return Color{.r = 128.0 / 255.0, .g = 128.0 / 255.0, .b = 128.0 / 255.0};
        } else if (name == "grey") {
            return Color{.r = 128.0 / 255.0, .g = 128.0 / 255.0, .b = 128.0 / 255.0};
        } else if (name == "green") {
            return Color{.r = 0.0 / 255.0, .g = 128.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "greenyellow") {
            return Color{.r = 173.0 / 255.0, .g = 255.0 / 255.0, .b = 47.0 / 255.0};
        } else if (name == "honeydew") {
            return Color{.r = 240.0 / 255.0, .g = 255.0 / 255.0, .b = 240.0 / 255.0};
        } else if (name == "hotpink") {
            return Color{.r = 255.0 / 255.0, .g = 105.0 / 255.0, .b = 180.0 / 255.0};
        } else if (name == "indianred") {
            return Color{.r = 205.0 / 255.0, .g = 92.0 / 255.0, .b = 92.0 / 255.0};
        } else if (name == "indigo") {
            return Color{.r = 75.0 / 255.0, .g = 0.0 / 255.0, .b = 130.0 / 255.0};
        } else if (name == "ivory") {
            return Color{.r = 255.0 / 255.0, .g = 255.0 / 255.0, .b = 240.0 / 255.0};
        } else if (name == "khaki") {
            return Color{.r = 240.0 / 255.0, .g = 230.0 / 255.0, .b = 140.0 / 255.0};
        } else if (name == "lavender") {
            return Color{.r = 230.0 / 255.0, .g = 230.0 / 255.0, .b = 250.0 / 255.0};
        } else if (name == "lavenderblush") {
            return Color{.r = 255.0 / 255.0, .g = 240.0 / 255.0, .b = 245.0 / 255.0};
        } else if (name == "lawngreen") {
            return Color{.r = 124.0 / 255.0, .g = 252.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "lemonchiffon") {
            return Color{.r = 255.0 / 255.0, .g = 250.0 / 255.0, .b = 205.0 / 255.0};
        } else if (name == "lightblue") {
            return Color{.r = 173.0 / 255.0, .g = 216.0 / 255.0, .b = 230.0 / 255.0};
        } else if (name == "lightcoral") {
            return Color{.r = 240.0 / 255.0, .g = 128.0 / 255.0, .b = 128.0 / 255.0};
        } else if (name == "lightcyan") {
            return Color{.r = 224.0 / 255.0, .g = 255.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "lightgoldenrodyellow") {
            return Color{.r = 250.0 / 255.0, .g = 250.0 / 255.0, .b = 210.0 / 255.0};
        } else if (name == "lightgray") {
            return Color{.r = 211.0 / 255.0, .g = 211.0 / 255.0, .b = 211.0 / 255.0};
        } else if (name == "lightgreen") {
            return Color{.r = 144.0 / 255.0, .g = 238.0 / 255.0, .b = 144.0 / 255.0};
        } else if (name == "lightgrey") {
            return Color{.r = 211.0 / 255.0, .g = 211.0 / 255.0, .b = 211.0 / 255.0};
        } else if (name == "lightpink") {
            return Color{.r = 255.0 / 255.0, .g = 182.0 / 255.0, .b = 193.0 / 255.0};
        } else if (name == "lightsalmon") {
            return Color{.r = 255.0 / 255.0, .g = 160.0 / 255.0, .b = 122.0 / 255.0};
        } else if (name == "lightseagreen") {
            return Color{.r = 32.0 / 255.0, .g = 178.0 / 255.0, .b = 170.0 / 255.0};
        } else if (name == "lightskyblue") {
            return Color{.r = 135.0 / 255.0, .g = 206.0 / 255.0, .b = 250.0 / 255.0};
        } else if (name == "lightslategray") {
            return Color{.r = 119.0 / 255.0, .g = 136.0 / 255.0, .b = 153.0 / 255.0};
        } else if (name == "lightslategrey") {
            return Color{.r = 119.0 / 255.0, .g = 136.0 / 255.0, .b = 153.0 / 255.0};
        } else if (name == "lightsteelblue") {
            return Color{.r = 176.0 / 255.0, .g = 196.0 / 255.0, .b = 222.0 / 255.0};
        } else if (name == "lightyellow") {
            return Color{.r = 255.0 / 255.0, .g = 255.0 / 255.0, .b = 224.0 / 255.0};
        } else if (name == "lime") {
            return Color{.r = 0.0 / 255.0, .g = 255.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "limegreen") {
            return Color{.r = 50.0 / 255.0, .g = 205.0 / 255.0, .b = 50.0 / 255.0};
        } else if (name == "linen") {
            return Color{.r = 250.0 / 255.0, .g = 240.0 / 255.0, .b = 230.0 / 255.0};
        } else if (name == "magenta") {
            return Color{.r = 255.0 / 255.0, .g = 0.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "maroon") {
            return Color{.r = 128.0 / 255.0, .g = 0.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "mediumaquamarine") {
            return Color{.r = 102.0 / 255.0, .g = 205.0 / 255.0, .b = 170.0 / 255.0};
        } else if (name == "mediumblue") {
            return Color{.r = 0.0 / 255.0, .g = 0.0 / 255.0, .b = 205.0 / 255.0};
        } else if (name == "mediumorchid") {
            return Color{.r = 186.0 / 255.0, .g = 85.0 / 255.0, .b = 211.0 / 255.0};
        } else if (name == "mediumpurple") {
            return Color{.r = 147.0 / 255.0, .g = 112.0 / 255.0, .b = 219.0 / 255.0};
        } else if (name == "mediumseagreen") {
            return Color{.r = 60.0 / 255.0, .g = 179.0 / 255.0, .b = 113.0 / 255.0};
        } else if (name == "mediumslateblue") {
            return Color{.r = 123.0 / 255.0, .g = 104.0 / 255.0, .b = 238.0 / 255.0};
        } else if (name == "mediumspringgreen") {
            return Color{.r = 0.0 / 255.0, .g = 250.0 / 255.0, .b = 154.0 / 255.0};
        } else if (name == "mediumturquoise") {
            return Color{.r = 72.0 / 255.0, .g = 209.0 / 255.0, .b = 204.0 / 255.0};
        } else if (name == "mediumvioletred") {
            return Color{.r = 199.0 / 255.0, .g = 21.0 / 255.0, .b = 133.0 / 255.0};
        } else if (name == "midnightblue") {
            return Color{.r = 25.0 / 255.0, .g = 25.0 / 255.0, .b = 112.0 / 255.0};
        } else if (name == "mintcream") {
            return Color{.r = 245.0 / 255.0, .g = 255.0 / 255.0, .b = 250.0 / 255.0};
        } else if (name == "mistyrose") {
            return Color{.r = 255.0 / 255.0, .g = 228.0 / 255.0, .b = 225.0 / 255.0};
        } else if (name == "moccasin") {
            return Color{.r = 255.0 / 255.0, .g = 228.0 / 255.0, .b = 181.0 / 255.0};
        } else if (name == "navajowhite") {
            return Color{.r = 255.0 / 255.0, .g = 222.0 / 255.0, .b = 173.0 / 255.0};
        } else if (name == "navy") {
            return Color{.r = 0.0 / 255.0, .g = 0.0 / 255.0, .b = 128.0 / 255.0};
        } else if (name == "oldlace") {
            return Color{.r = 253.0 / 255.0, .g = 245.0 / 255.0, .b = 230.0 / 255.0};
        } else if (name == "olive") {
            return Color{.r = 128.0 / 255.0, .g = 128.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "olivedrab") {
            return Color{.r = 107.0 / 255.0, .g = 142.0 / 255.0, .b = 35.0 / 255.0};
        } else if (name == "orange") {
            return Color{.r = 255.0 / 255.0, .g = 165.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "orangered") {
            return Color{.r = 255.0 / 255.0, .g = 69.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "orchid") {
            return Color{.r = 218.0 / 255.0, .g = 112.0 / 255.0, .b = 214.0 / 255.0};
        } else if (name == "palegoldenrod") {
            return Color{.r = 238.0 / 255.0, .g = 232.0 / 255.0, .b = 170.0 / 255.0};
        } else if (name == "palegreen") {
            return Color{.r = 152.0 / 255.0, .g = 251.0 / 255.0, .b = 152.0 / 255.0};
        } else if (name == "paleturquoise") {
            return Color{.r = 175.0 / 255.0, .g = 238.0 / 255.0, .b = 238.0 / 255.0};
        } else if (name == "palevioletred") {
            return Color{.r = 219.0 / 255.0, .g = 112.0 / 255.0, .b = 147.0 / 255.0};
        } else if (name == "papayawhip") {
            return Color{.r = 255.0 / 255.0, .g = 239.0 / 255.0, .b = 213.0 / 255.0};
        } else if (name == "peachpuff") {
            return Color{.r = 255.0 / 255.0, .g = 218.0 / 255.0, .b = 185.0 / 255.0};
        } else if (name == "peru") {
            return Color{.r = 205.0 / 255.0, .g = 133.0 / 255.0, .b = 63.0 / 255.0};
        } else if (name == "pink") {
            return Color{.r = 255.0 / 255.0, .g = 192.0 / 255.0, .b = 203.0 / 255.0};
        } else if (name == "plum") {
            return Color{.r = 221.0 / 255.0, .g = 160.0 / 255.0, .b = 221.0 / 255.0};
        } else if (name == "powderblue") {
            return Color{.r = 176.0 / 255.0, .g = 224.0 / 255.0, .b = 230.0 / 255.0};
        } else if (name == "purple") {
            return Color{.r = 128.0 / 255.0, .g = 0.0 / 255.0, .b = 128.0 / 255.0};
        } else if (name == "red") {
            return Color{.r = 255.0 / 255.0, .g = 0.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "rosybrown") {
            return Color{.r = 188.0 / 255.0, .g = 143.0 / 255.0, .b = 143.0 / 255.0};
        } else if (name == "royalblue") {
            return Color{.r = 65.0 / 255.0, .g = 105.0 / 255.0, .b = 225.0 / 255.0};
        } else if (name == "saddlebrown") {
            return Color{.r = 139.0 / 255.0, .g = 69.0 / 255.0, .b = 19.0 / 255.0};
        } else if (name == "salmon") {
            return Color{.r = 250.0 / 255.0, .g = 128.0 / 255.0, .b = 114.0 / 255.0};
        } else if (name == "sandybrown") {
            return Color{.r = 244.0 / 255.0, .g = 164.0 / 255.0, .b = 96.0 / 255.0};
        } else if (name == "seagreen") {
            return Color{.r = 46.0 / 255.0, .g = 139.0 / 255.0, .b = 87.0 / 255.0};
        } else if (name == "seashell") {
            return Color{.r = 255.0 / 255.0, .g = 245.0 / 255.0, .b = 238.0 / 255.0};
        } else if (name == "sienna") {
            return Color{.r = 160.0 / 255.0, .g = 82.0 / 255.0, .b = 45.0 / 255.0};
        } else if (name == "silver") {
            return Color{.r = 192.0 / 255.0, .g = 192.0 / 255.0, .b = 192.0 / 255.0};
        } else if (name == "skyblue") {
            return Color{.r = 135.0 / 255.0, .g = 206.0 / 255.0, .b = 235.0 / 255.0};
        } else if (name == "slateblue") {
            return Color{.r = 106.0 / 255.0, .g = 90.0 / 255.0, .b = 205.0 / 255.0};
        } else if (name == "slategray") {
            return Color{.r = 112.0 / 255.0, .g = 128.0 / 255.0, .b = 144.0 / 255.0};
        } else if (name == "slategrey") {
            return Color{.r = 112.0 / 255.0, .g = 128.0 / 255.0, .b = 144.0 / 255.0};
        } else if (name == "snow") {
            return Color{.r = 255.0 / 255.0, .g = 250.0 / 255.0, .b = 250.0 / 255.0};
        } else if (name == "springgreen") {
            return Color{.r = 0.0 / 255.0, .g = 255.0 / 255.0, .b = 127.0 / 255.0};
        } else if (name == "steelblue") {
            return Color{.r = 70.0 / 255.0, .g = 130.0 / 255.0, .b = 180.0 / 255.0};
        } else if (name == "tan") {
            return Color{.r = 210.0 / 255.0, .g = 180.0 / 255.0, .b = 140.0 / 255.0};
        } else if (name == "teal") {
            return Color{.r = 0.0 / 255.0, .g = 128.0 / 255.0, .b = 128.0 / 255.0};
        } else if (name == "thistle") {
            return Color{.r = 216.0 / 255.0, .g = 191.0 / 255.0, .b = 216.0 / 255.0};
        } else if (name == "tomato") {
            return Color{.r = 255.0 / 255.0, .g = 99.0 / 255.0, .b = 71.0 / 255.0};
        } else if (name == "turquoise") {
            return Color{.r = 64.0 / 255.0, .g = 224.0 / 255.0, .b = 208.0 / 255.0};
        } else if (name == "violet") {
            return Color{.r = 238.0 / 255.0, .g = 130.0 / 255.0, .b = 238.0 / 255.0};
        } else if (name == "wheat") {
            return Color{.r = 245.0 / 255.0, .g = 222.0 / 255.0, .b = 179.0 / 255.0};
        } else if (name == "white") {
            return Color{.r = 255.0 / 255.0, .g = 255.0 / 255.0, .b = 255.0 / 255.0};
        } else if (name == "whitesmoke") {
            return Color{.r = 245.0 / 255.0, .g = 245.0 / 255.0, .b = 245.0 / 255.0};
        } else if (name == "yellow") {
            return Color{.r = 255.0 / 255.0, .g = 255.0 / 255.0, .b = 0.0 / 255.0};
        } else if (name == "yellowgreen") {
            return Color{.r = 154.0 / 255.0, .g = 205.0 / 255.0, .b = 50.0 / 255.0};
        }

        JSON_ERR("Unknown color name: %s", name.c_str());
    };

    Color vertex_fill = name_to_color("black");
    float vertex_fill_opacity = 1.0;
    Color vertex_stroke = name_to_color("black");
    float vertex_stroke_opacity = 1.0;
    float vertex_stroke_width = 1;

    Color edge_fill = name_to_color("transparent");
    float edge_fill_opacity = 0.0;
    Color edge_stroke = name_to_color("black");
    float edge_stroke_opacity = 1.0;
    float edge_stroke_width = 1;
    std::string edge_stroke_dash = "none";
    bool edge_hull = false;

    {
        if (json.contains("vertex-fill") && json["vertex-fill"].is_string())
            vertex_fill = name_to_color(json["vertex-fill"].get<std::string>());

        if (json.contains("vertex-fill-opacity") && json["vertex-fill-opacity"].is_number())
            vertex_fill_opacity = json["vertex-fill-opacity"].get<float>();

        if (json.contains("vertex-stroke") && json["vertex-stroke"].is_string())
            vertex_stroke = name_to_color(json["vertex-stroke"].get<std::string>());

        if (json.contains("vertex-stroke-opacity") && json["vertex-stroke-opacity"].is_number())
            vertex_stroke_opacity = json["vertex-stroke-opacity"].get<float>();

        if (json.contains("vertex-stroke-width") && json["vertex-stroke-width"].is_number())
            vertex_stroke_width = json["vertex-stroke-width"].get<float>();
    }

    {
        if (json.contains("edge-fill") && json["edge-fill"].is_string())
            edge_fill = name_to_color(json["edge-fill"].get<std::string>());

        if (json.contains("edge-fill-opacity") && json["edge-fill-opacity"].is_number())
            edge_fill_opacity = json["edge-fill-opacity"].get<float>();

        if (json.contains("edge-stroke") && json["edge-stroke"].is_string())
            edge_stroke = name_to_color(json["edge-stroke"].get<std::string>());

        if (json.contains("edge-stroke-opacity") && json["edge-stroke-opacity"].is_number())
            edge_stroke_opacity = json["edge-stroke-opacity"].get<float>();

        if (json.contains("edge-stroke-width") && json["edge-stroke-width"].is_number())
            edge_stroke_width = json["edge-stroke-width"].get<float>();

        if (json.contains("edge-stroke-dash") && json["edge-stroke-dash"].is_string())
            edge_stroke_dash = json["edge-stroke-dash"].get<std::string>();

        if (json.contains("edge-convex-hull") && json["edge-convex-hull"].is_boolean())
            edge_hull = json["edge-convex-hull"].get<bool>();
    }

    bounds.size = bounds.max - bounds.min;

    output.begin(Vec2f{bounds.min.x - padding.left, bounds.min.y - padding.top}, Vec2f{bounds.size.x + padding.left + padding.right, bounds.size.y + padding.top + padding.bottom});

    for (auto e : edges) {
        Color cur_edge_fill = edge_fill;
        float cur_edge_fill_opacity = edge_fill_opacity;
        Color cur_edge_stroke = edge_stroke;
        float cur_edge_stroke_opacity = edge_stroke_opacity;
        float cur_edge_stroke_width = edge_stroke_width;
        std::string cur_edge_stroke_dash = edge_stroke_dash;

        float cur_edge_draw_radius = edge_draw_radius;

        bool cur_edge_hull = edge_hull;

        if (e.json.contains("fill") && e.json["fill"].is_string())
            cur_edge_fill = name_to_color(e.json["fill"].get<std::string>());

        if (e.json.contains("fill-opacity") && e.json["fill-opacity"].is_number())
            cur_edge_fill_opacity = e.json["fill-opacity"].get<float>();

        if (e.json.contains("stroke") && e.json["stroke"].is_string())
            cur_edge_stroke = name_to_color(e.json["stroke"].get<std::string>());

        if (e.json.contains("stroke-opacity") && e.json["stroke-opacity"].is_number())
            cur_edge_stroke_opacity = e.json["stroke-opacity"].get<float>();

        if (e.json.contains("stroke-width") && e.json["stroke-width"].is_number())
            cur_edge_stroke_width = e.json["stroke-width"].get<float>();

        if (e.json.contains("radius") && e.json["radius"].is_number())
            cur_edge_draw_radius = e.json["radius"].get<float>();

        if (e.json.contains("dash") && e.json["dash"].is_string())
            cur_edge_stroke_dash = e.json["dash"].get<std::string>();

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
                new_verts.push_back(edge_verts[cur_index]);

                size_t best = (cur_index + 1) % edge_verts.size();

                for (int i = 0; i < edge_verts.size(); i++) {
                    if (orientation(vertices[edge_verts[cur_index]].pos, vertices[edge_verts[i]].pos, vertices[edge_verts[best]].pos) == COUNTERCLOCKWISE)
                        best = i;
                }

                cur_index = best;
            } while (cur_index != min_index);

            edge_verts = new_verts;
        }

        Vec2f mean = [&]() {
            Vec2f sum = {};
            for (auto v : e.vertices)
                sum += vertices[v].pos;
            return sum / (float)e.vertices.size();
        }();

        // Sort the vertices in clockwise order
        std::sort(edge_verts.begin(), edge_verts.end(), [&](size_t a, size_t b) {
            auto a_pos = vertices[a].pos - mean;
            auto b_pos = vertices[b].pos - mean;

            auto a_ang = std::atan2(a_pos.y, a_pos.x);
            auto b_ang = std::atan2(b_pos.y, b_pos.x);

            return a_ang < b_ang;
        });

        auto edge_line = [&](Vec2f a, Vec2f b, Vec2f c, bool first, auto move, auto line, auto arc) {
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
                float a1 = o1.y - p1.y;
                float b1 = p1.x - o1.x;
                float c1 = a1 * (p1.x) + b1 * (p1.y);

                float a2 = o2.y - p2.y;
                float b2 = p2.x - o2.x;
                float c2 = a2 * (p2.x) + b2 * (p2.y);

                auto determinant = a1 * b2 - a2 * b1;

                Vec2f intersect{(b2 * c1 - b1 * c2) / determinant, (a1 * c2 - a2 * c1) / determinant};

                if (determinant < 0.0001) {
                    intersect = (p1 + p2) / 2.0f;
                }

                auto x1 = p1 + (intersect - p1) * 2;
                auto x2 = p2 + (intersect - p2) * 2;

                if (first) {
                    move(x2);
                } else {
                    line(x1);
                    arc(x2, Vec2f{cur_edge_draw_radius, cur_edge_draw_radius}, 0, 0);
                }

            } else {

                auto p1_ang = std::atan2((p1 - b).y, (p1 - b).x);
                auto p2_ang = std::atan2((p2 - b).y, (p2 - b).x);

                int large_arc = p2_ang - p1_ang < M_PI ? 0 : 1;

                if (first) {
                    move(p2);
                } else {
                    line(p1);
                    arc(p2, Vec2f{cur_edge_draw_radius, cur_edge_draw_radius}, large_arc, 1);
                }
            }
        };

        if (edge_verts.size() == 1) {
            output.circle(vertices[edge_verts[0]].pos, cur_edge_draw_radius, cur_edge_fill, cur_edge_fill_opacity, cur_edge_stroke, cur_edge_stroke_opacity, cur_edge_stroke_width);
        } else if (edge_verts.size() == 2) {
            output.path(cur_edge_fill,
                        cur_edge_fill_opacity,
                        cur_edge_stroke,
                        cur_edge_stroke_opacity,
                        cur_edge_stroke_width,
                        cur_edge_stroke_dash,
                        (e.json.contains("label") && e.json["label"].is_string()) ? std::optional<std::string>(e.json["label"].get<std::string>()) : std::nullopt,
                        [&](auto move, auto line, auto arc) {
                            edge_line(vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, vertices[edge_verts[0]].pos, true, move, line, arc);
                            edge_line(vertices[edge_verts[1]].pos, vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, false, move, line, arc);
                            edge_line(vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, vertices[edge_verts[0]].pos, false, move, line, arc);
                        });
        } else if (edge_verts.size() >= 2) {
            output.path(cur_edge_fill,
                        cur_edge_fill_opacity,
                        cur_edge_stroke,
                        cur_edge_stroke_opacity,
                        cur_edge_stroke_width,
                        cur_edge_stroke_dash,
                        (e.json.contains("label") && e.json["label"].is_string()) ? std::optional<std::string>(e.json["label"].get<std::string>()) : std::nullopt,
                        [&](auto move, auto line, auto arc) {
                            auto prevprev = vertices[edge_verts[0]].pos;
                            auto prev = vertices[edge_verts[1]].pos;

                            edge_line(prevprev, prev, vertices[edge_verts[2]].pos, true, move, line, arc);

                            prevprev = prev;
                            prev = vertices[edge_verts[2]].pos;

                            for (auto i = 3; i < edge_verts.size(); i++) {
                                auto cur = vertices[edge_verts[i]].pos;
                                edge_line(prevprev, prev, cur, false, move, line, arc);
                                prevprev = prev;
                                prev = cur;
                            }
                            edge_line(prevprev, prev, vertices[edge_verts[0]].pos, false, move, line, arc);
                            edge_line(prev, vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, false, move, line, arc);
                            edge_line(vertices[edge_verts[0]].pos, vertices[edge_verts[1]].pos, vertices[edge_verts[2]].pos, false, move, line, arc);
                        });
        }
    }

    for (auto v : vertices) {
        Color cur_vertex_fill = vertex_fill;
        float cur_vertex_fill_opacity = vertex_fill_opacity;
        Color cur_vertex_stroke = vertex_stroke;
        float cur_vertex_stroke_opacity = vertex_stroke_opacity;
        float cur_vertex_stroke_width = vertex_stroke_width;

        float cur_vertex_radius = vertex_radius;

        if (v.json.contains("radius") && v.json["radius"].is_number())
            cur_vertex_radius = v.json["radius"].get<float>();

        if (v.json.contains("fill") && v.json["fill"].is_string())
            cur_vertex_fill = name_to_color(v.json["fill"].get<std::string>());

        if (v.json.contains("fill-opacity") && v.json["fill-opacity"].is_number())
            cur_vertex_fill_opacity = v.json["fill-opacity"].get<float>();

        if (v.json.contains("stroke") && v.json["stroke"].is_string())
            cur_vertex_stroke = name_to_color(v.json["stroke"].get<std::string>());

        if (v.json.contains("stroke-opacity") && v.json["stroke-opacity"].is_number())
            cur_vertex_stroke_opacity = v.json["stroke-opacity"].get<float>();

        if (v.json.contains("stroke-width") && v.json["stroke-width"].is_number())
            cur_vertex_stroke_width = v.json["stroke-width"].get<float>();

        output.circle(v.pos,
                      cur_vertex_radius,
                      cur_vertex_fill,
                      cur_vertex_fill_opacity,
                      cur_vertex_stroke,
                      cur_vertex_stroke_opacity,
                      cur_vertex_stroke_width,
                      (v.json.contains("label") && v.json["label"].is_string()) ? std::optional<std::string>(v.json["label"].get<std::string>()) : std::nullopt);
    }

    output.end();
}

int main(int argc, char** argv) {

    nlohmann::json json;

    std::optional<std::string> input_file;

    if (argc > 1) {
    }

    enum { SVG, TIKZ } format = SVG;

    if (argc > 1) {
        int pos = 1;
        while (pos < argc) {
            if (std::string(argv[pos]).starts_with("--")) {
                if (std::string(argv[pos]) == "--format") {
                    pos++;

                    if (std::string(argv[pos]) == "svg") {
                        format = SVG;
                    } else if (std::string(argv[pos]) == "tikz") {
                        format = TIKZ;
                    } else {
                        JSON_ERR("Unknown format: %s", argv[pos]);
                    }

                    pos++;
                } else {
                    JSON_ERR("Unknown argument: %s", argv[pos]);
                }
            } else {
                input_file = argv[pos];
                break;
            }
        }
    }

    if (input_file.has_value()) {
        std::ifstream in(input_file.value());
        if (!in)
            JSON_ERR("Could not read input file: %s", input_file.value().c_str());
        in >> json;
    } else {
        std::cin >> json;
    }

    switch (format) {
    case SVG: {
        SVGOutput svg_output;
        draw(json, svg_output);
    } break;
    case TIKZ: {
        TIKZOutput tikz_output;
        draw(json, tikz_output);
    } break;
    }
}
