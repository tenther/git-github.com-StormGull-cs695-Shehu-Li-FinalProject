#include "wykobi.hpp"
#include "wykobi_graphics_opengl.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace std;
namespace pt = boost::property_tree;

typedef float ord_type;
typedef vector<ord_type> position_vec;
typedef pair<ord_type, ord_type> coordinate;
typedef vector<coordinate> coordinate_vec;
typedef wykobi::point2d<ord_type> Point;
typedef wykobi::circle<ord_type> Circle;
typedef wykobi::line<ord_type,2> Line;
typedef vector<Point> Points;

#define to_string boost::lexical_cast<string>
#define to_float boost::lexical_cast<ord_type>
#define make_point wykobi::make_point<ord_type>

bool compare_points_y(Point& p1, Point& p2) {
    return p1.y < p2.y;
}

void read_positions(istream& in, position_vec& positions){
    string s;
    size_t num = 0;
    ord_type d;
    while(in >> s) {
        num++;
        try {
            d = to_float(s);
        }
        catch(const boost::bad_lexical_cast &)
        {
            throw("Couldn't convert " + s +  " to floating point.");
        }
        positions.push_back(d);
    }
}

template <typename T> void add_list_to_ptree(pt::ptree& parent, string child_name, vector<T> p) {
    pt::ptree child_node;
    for(typename vector<T>::iterator it = p.begin();
        it != p.end();
        ++it) {
        pt::ptree node;
        // cerr << "Adding node " << " -> " << *it << endl;
        node.put("", *it);
        child_node.push_back(make_pair("", node));
    }
    parent.add_child(child_name, child_node);
    // cerr << endl;

}

int main(int argc, char** argv) {
    position_vec positions;
    Points points;
    pt::ptree root;

    try {
        if(argc != 2) {
            throw("Expected one file name on the command line.");
        }

        ifstream in(argv[1]);
        if(!in) {
            throw("Could not open file " + to_string(argv[1]) + ".");
        }

        read_positions(in, positions);
        add_list_to_ptree(root, string("input_positions"), positions);

        if(positions.size()%2 != 0) {
            throw("Expected and even number of numbers. Not " + to_string(positions.size()));
        }

        size_t num_points = positions.size()/2;

        points.reserve(num_points);
        for(size_t i = 0; i < num_points; i++) {
            points.push_back(make_point(positions[2*i], positions[2*i+1]));
        }

        add_list_to_ptree(root, string("input_points"), points);

        pt::ptree solutions;
        for(size_t i = 0; i < points.size(); i++) {
            for(size_t j = 0; j < points.size(); j++) {
                if( j > i ) {
                    Point mid_point = wykobi::segment_mid_point(points[i], points[j]);
                    ord_type max_distance = -1;
                    for(Points::iterator k = points.begin(); k != points.end(); ++k) {
                        ord_type d = wykobi::distance(mid_point, *k);
                        if(d > max_distance) {
                            max_distance = d;
                        }
                    }
                    ord_type radius = max_distance + 1;
                    Circle circle = wykobi::make_circle(mid_point, radius);
                    Line   diameter_line = wykobi::make_line(points[i], points[j]);

                    std::vector<wykobi::point2d<ord_type> > intersections;
                    intersection_point(diameter_line, circle, back_inserter(intersections));
                    Point intersection = *max_element(intersections.begin(), intersections.end(), compare_points_y);
                    Line tangent = tangent_line(circle, intersection);
                    Points st_numbering;

//                    st_numbering.reserve(points.size());
                    for(Points::iterator k = points.begin(); k != points.end(); ++k) {
                        Point close_point = wykobi::closest_point_on_line_from_point(tangent, *k);
                        // cerr << "Point " << *k << " -> " << close_point << endl;
                        st_numbering.push_back(close_point);
                    }
                    // cerr << endl;
                    // add objects to json object
                    pt::ptree solution_node;
                    solution_node.put("circle.center", mid_point);
                    solution_node.put("circle.radius", radius);
                    solution_node.put("intersection", intersection);
                    solution_node.put("tangent", tangent);
                    // add_ptree_point_list(solution_node, "st_numbering", st_numbering);
                    add_list_to_ptree(solution_node, "st_numbering", st_numbering);

                    // pt::ptree node;
                    // node.put("", solution_node);
                    solutions.push_back(make_pair("", solution_node));
                }
            }
        }

        root.add_child("solutions", solutions);
        ofstream of("../st-numbering");
        pt::write_json(of, root);
    }

    catch(string& s) {
        cerr << "ERROR: " << s << endl;
        return(EXIT_FAILURE);
    }


    return(0);
}

