# Hypergraph Drawing
Programs for layout and drawing of general hypergraphs.

Split between two programs.
hypergraph-layout accepts a json description of the hypergraph and uses clique expansion and graphviz to layout the graph.
hypergraph-draw takes the hypergraph json with coordinates for the verticies and renders the graph as a svg file.

## General Usage
By default the program accepts input on stdin and write output to stdout.
If an argument is supplied that argument is used as the input file and output is stil to standard output.


For example if the following json is written to a file called hypergraph.json.
```
{
    "vertex-fill": "white",
    "edge-fill-opacity": 0.3,
    "vertices": {
        "3": {"fill": "blue"}
    },
    "edges": [
        {"vertices": [0, 1, 2], "fill": "blue"},
        {"vertices": [0, 3, 4], "fill": "green"},
        {"vertices": [3, 4, 5], "fill": "red"},
        {"vertices": [4, 5, 8], "fill": "red"},
        {"vertices": [2, 6, 7], "fill": "blue"},
        {"vertices": [7, 6, 8], "fill": "green"}
    ]
}
```

The following command will layout and draw the graph and write the output to a file called hypergraph.svg.
```
cat hypergraph.json | ./hypergraph-layout | ./hypergraph-draw > hypergraph.svg
```

Producing the following:

![alt text](https://github.com/tommy1019/hypergraph-draw/blob/master/example.svg?raw=true)

## How to build
nlohmann/json is required for both layout and drawing.
Graphviz is required for layout only.

To build:
```
mkdir build && cd build
cmake ..
make
```

## JSON Structure
Hypergraphs are input to the program as JSON.

The only required field in the top level object is "edges".
This field must be an array of objects.
Each edge object must contain a "vertices" field that is a list of integers.

Optionally a "vertices" field may be specified at the top level.
This field may either be an dictinary of integers to objects or an array of objects.
The verticies field of each hyperedge indexes into this field creating a default vertex if none is present.

Additionally any of the global, vertex, or hyperedge options may be specified in their respective places.

## Layout Options
The only option for layout is "layout-engine" which can be set to any engine for graphviz.
The default is neato.

## Drawing options
### Global Options
These options are set at the top level in the json and set defaults for all relevant fields.
| Option        | Description   | Default       |
| ------------- | ------------- | ------------- |
| padding-top | Padding added above all vertices in the graph | 30 |
| padding-bottom | Padding added below all vertices in the graph | 30 |
| padding-left | Padding added to the left of all vertices in the graph | 30 |
| padding-right | Padding added to the right of all vertices in the graph | 30 |
| vertex-radius | Radius of each vertex | 12 |
| vertex-fill | Color to fill each vertex | black |
| vertex-fill-opacity | Opacity for the fill of each vertex. Float from 0 to 1 | 1.0 |
| vertex-stroke | Color to draw the outline of each vertex | black |
| vertex-stroke-opacity | Opacity for the outline of each vertex. Float from 0 to 1 | 1.0 |
| vertex-stroke-width | Thickness of the outline for each vertex | 1.0 |
| edge-draw-radius | Distance from each vertex to draw each hyperedge | 18 |
| edge-fill | Color to fill each vertex | transparent |
| edge-fill-opacity | Opacity for the fill of each edge. Float from 0 to 1 | 0.0 |
| edge-stroke | Color to draw the outline of each edge | black |
| edge-stroke-opacity | Opacity for the outline of each edge. Float from 0 to 1 | 1.0 |
| edge-stroke-width | Thickness of the outline for each edge | 1.0 |
| edge-convex-hull | True to use the convex hull of vertices in the hyperedge instead of drawing a non-convex shape | false |

### Vertex Options
These options are set per-vertex and override options set globally.
| Option        | Description   | Default       |
| ------------- | ------------- | ------------- |
| label | Label text to draw inside of the vertex |   |
| radius | Radius of the vertex | vertex-radius |
| fill | Color to fill the vertex | vertex-fill |
| fill-opacity | Opacity for the fill of the vertex. Float from 0 to 1 | vertex-fill-opacity |
| stroke | Color to draw the outline of the vertex | vertex-stroke |
| stroke-opacity | Opacity for the outline of the vertex. Float from 0 to 1 | vertex-stroke-opacity |
| stroke-width | Thickness of the outline for the vertex | vertex-stroke-width |

### Hyperedge Options
These options are set per-hyperedge and override options set globally.
| Option        | Description   | Default       |
| ------------- | ------------- | ------------- |
| label | Label text to draw inside of the hyperedge |   |
| radius | Distance from each vertex to draw the hyperedge | edge-radius |
| fill | Color to fill the hyperedge | edge-fill |
| fill-opacity | Opacity for the fill of the hyperedge. Float from 0 to 1 | edge-fill-opacity |
| stroke | Color to draw the outline of the hyperedge | edge-stroke |
| stroke-opacity | Opacity for the outline of the hyperedge. Float from 0 to 1 | edge-stroke-opacity |
| stroke-width | Thickness of the outline for the hyperedge | edge-stroke-width |
