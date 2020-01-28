#include <string>
#include <cmath>
#include <map>
#include "Bridges.h"
#include "DataSource.h"
#include "GraphAdjList.h"
#include "classification.h"

using namespace std;
using namespace bridges;


///returns the total number of vertices in that branch of the tree
int count_vertices(GraphAdjList<string, VertexData> * classification_tree,  const string& root) {
  //this assumes that root exists
  
  int count = 1; // myself

  for (auto & e : classification_tree->outgoingEdgeSetOf(root)) {
    auto to = e.to();
    count += count_vertices(classification_tree, to);
  }
  
  return count;
}

///returns the number of hits in that branch of the tree (sum of all hits)
int count_hit_rec(GraphAdjList<string, VertexData> * classification_tree,
		  const string& root) {
  //this assumes that root exists
  
  int count = classification_tree->getVertexData(root).hits; // myself

  for (auto & e : classification_tree->outgoingEdgeSetOf(root)) {
    auto to = e.to();
    count += count_hit_rec(classification_tree, to);
  }
  
  return count;
}

///returns the maximum of hits a branch sees per level. In other words the return value is a map  so that map[2] is the branch of level 2 with the highest hit
std::map<int,int> count_hit_per_level_rec(GraphAdjList<string, VertexData> * classification_tree,
					  const string& root,
					  int depth = 0) {
  //this assumes that root exists
  std::map<int, int> retval;
  
  int count = count_hit_rec(classification_tree, root);

  retval[depth] = count;
  
  for (auto & e : classification_tree->outgoingEdgeSetOf(root)) {
    auto to = e.to();
    auto children =  count_hit_per_level_rec(classification_tree, to, depth+1);
    for (auto p : children) {
      if (p.second > retval[p.first])
	retval[p.first] = p.second;
    }
  }
  
  return retval;
}

///returns the maximum of hits a branch sees per level. In other words the return value is a map  so that map[2] is the branch of level 2 with the highest hit
std::map<int,int> max_cnt12_per_level_rec(GraphAdjList<string, VertexData> * classification_tree,
					  const string& root,
					  int depth = 0) {
  //this assumes that root exists
  std::map<int, int> retval;

  VertexData vd = classification_tree->getVertexData(root);
  
  int count = std::max(vd.cnt1, vd.cnt2);

  retval[depth] = count;
  
  for (auto & e : classification_tree->outgoingEdgeSetOf(root)) {
    auto to = e.to();
    auto children =  max_cnt12_per_level_rec(classification_tree, to, depth+1);
    for (auto p : children) {
      if (p.second > retval[p.first])
	retval[p.first] = p.second;
    }
  }
  
  return retval;
}




///This scales the vertices based on downstream hits
void scale_intermediary(GraphAdjList<string, VertexData> * classification_tree,  const string& root) {

  std::map<int,int> hitmap = count_hit_per_level_rec(classification_tree,
						     root, 0);
 
  
  std::function<void(const string&, int )> helper
    = [&helper, &hitmap, &classification_tree] (const string& root, int depth = 0) {
    int count  = count_hit_rec (classification_tree, root);
    
    {
      auto v = classification_tree->getVertex(root);
      
      std::string l = v->getLabel();
      l+= "\n\t(downstreamcount: " + std::to_string(count) + ")";
      v->setLabel(l);
      
      double size = 5+(45.*((double)count)/hitmap[depth]);
      
      v->setSize((size>49?49:size));
    }
    
    for (auto & e : classification_tree->outgoingEdgeSetOf(root)) {
      auto to = e.to();
      helper(to,  depth+1);
    }
  };


  //This all assumes that the graph is actually a tree. If there is a cycle there will be infinite loops
  
  helper(root, 0);
}



///This is a basic radial layout algorithm
void layout_basic(GraphAdjList<string, VertexData> * classification_tree,  const string& root) {


  std::function<void(GraphAdjList<string, VertexData> *,
		     const string& ,
		     double, double,
		     int , double) > helper
    = [&helper] (GraphAdjList<string, VertexData> * classification_tree,
		 const string& root,
		 double angle_begin, double angle_end,
		 int depth, double depth_offset ) -> void {
    double angle_center = (angle_end-angle_begin)/2.+angle_begin;
    
    classification_tree->getVertex(root)->setLocation(depth_offset*depth*cos(angle_center),
						      depth_offset*depth*sin(angle_center));
    
    int local_count = count_vertices(classification_tree, root) - 1;
    
    //listing the neighboors in sorted order so that the layout always
    //list the vertices in the same order independent of how the
    //internal bridges datastructure might order them
    std::vector<std::string> neighboors;
    for (auto & e : classification_tree->outgoingEdgeSetOf(root)) {
      auto to = e.to();
      neighboors.push_back(to);
    }
    
    std::sort(neighboors.begin(), neighboors.end());
    
    double base_angle = angle_begin; 
    for (auto & to : neighboors) {
      int subcount = count_vertices(classification_tree, to);
      
      double fraction = (double)subcount/(double)local_count;
      double allocated_angle = fraction * (angle_end-angle_begin);
      
      helper(classification_tree, to,
	     base_angle, base_angle+allocated_angle,
	     depth+1, depth_offset);
      base_angle += allocated_angle;
    }
  };


  //This all assumes that the graph is actually a tree. If there is a cycle there will be infinite loops
  
  int howmany_nodes = count_vertices(classification_tree, root);

  std::cout<<"nodes: "<<howmany_nodes<<std::endl;

  double angle_per_vertex = 2*M_PI / howmany_nodes;

  double needed_length = howmany_nodes * 15.; // 15 so that you have 10 for the vertex and 5 of margin
  double depth_offset = needed_length / M_PI / 2. / 3.; // reversing circ = pi*diameter . most vertices are going to sit on a circle at depth 3.
  
  helper(classification_tree, root, 0, 2*M_PI, 0, depth_offset);
}



///This is a radial layout algorithm with multiple layers
void layout_radial_layered(GraphAdjList<string, VertexData> * classification_tree,  const string& root) {

  int layers = 3;
  double layer_depth = 15;

  //This all assumes that the graph is actually a tree. If there is a cycle there will be infinite loops
  
  int howmany_nodes = count_vertices(classification_tree, root);

  std::cout<<"nodes: "<<howmany_nodes<<std::endl;

  double angle_per_vertex = 2*M_PI / howmany_nodes;

  double needed_length = howmany_nodes * 15.; // 15 so that you have 10 for the vertex and 5 of margin
  needed_length /= layers; 
  double depth_offset = needed_length / M_PI / 2. / 3.; // reversing circ = pi*diameter . most vertices are going to sit on a circle at depth 3.

  
  std::function<void(GraphAdjList<string, VertexData> *,
		     const string& ,
		     double, double,
		     int, int) > helper
    = [&helper, layers, layer_depth, depth_offset]
    (GraphAdjList<string, VertexData> * classification_tree,
     const string& root,
     double angle_begin, double angle_end,
     int depth, int layer ) -> void {
    double angle_center = (angle_end-angle_begin)/2.+angle_begin;
    
    double local_radius = depth_offset*depth + layer*layer_depth;
    
    classification_tree->getVertex(root)->setLocation(local_radius*cos(angle_center),
						      local_radius*sin(angle_center));
    
    int local_count = count_vertices(classification_tree, root) - 1;
    
    //listing the neighboors in sorted order so that the layout always
    //list the vertices in the same order independent of how the
    //internal bridges datastructure might order them
    std::vector<std::string> neighboors;
    for (auto & e : classification_tree->outgoingEdgeSetOf(root)) {
      auto to = e.to();
      neighboors.push_back(to);
    }
    
    std::sort(neighboors.begin(), neighboors.end());
    
    double base_angle = angle_begin;
    int which_layer = 0;
    for (auto & to : neighboors) {
      int subcount = count_vertices(classification_tree, to);
      
      double fraction = (double)subcount/(double)local_count;
      double allocated_angle = fraction * (angle_end-angle_begin);
      
      helper(classification_tree, to,
	     base_angle, base_angle+allocated_angle,
	     depth+1, which_layer);
      
      base_angle += allocated_angle;
      which_layer = (which_layer + 1) % layers;
      
    }
  };


  
  helper(classification_tree, root, 0, 2*M_PI, 0, 0);
}



///this colors a comparison tree with diverging colorscale
void color_compare(GraphAdjList<string, VertexData> * compare_tree,  const string& root) {

  auto color_choice_ratio = [] (double ratio, double maxratio) {
    //picked from one of color brewer's diverging scale
    Color positive_color("#a50026");
    Color negative_color("#313695");
    Color white("#FFFFFF");

    //clamp ratio
    ratio = std::min(ratio, maxratio);
    ratio = std::max(ratio, 1./maxratio);

    //option 1 is to go logscale
    //double normalize_ratio = log(ratio)/log(maxratio); //in [-1;1]

    double normalize_ratio = 0;
    if (ratio > 1)
      normalize_ratio = ratio/maxratio; //in [-1;1]
    else
      normalize_ratio = - (1./ratio)/maxratio; //in [-1;1]
      
    //std::cerr<<"ratio: "<<ratio<<" normalize_ratio: "<<normalize_ratio<<"\n";
    
    double blend = normalize_ratio/2. + .5; //blending parameter in [0;1]
    
    Color ret;

    if (blend > .5) {
      double fracpos = (blend-.5)*2; //blend positive and white
      ret.setRed( (1-fracpos)*white.getRed() + (fracpos)*positive_color.getRed());
      ret.setGreen( (1-fracpos)*white.getGreen() + (fracpos)*positive_color.getGreen());
      ret.setBlue( (1-fracpos)*white.getBlue() + (fracpos)*positive_color.getBlue());
    }
    else {// blend < .5
      double fracneg = 1-blend*2 ; //blend white and negative
      ret.setRed( (fracneg)*negative_color.getRed() + (1-fracneg)*white.getRed());
      ret.setGreen( (fracneg)*negative_color.getGreen() + (1-fracneg)*white.getGreen());
      ret.setBlue( (fracneg)*negative_color.getBlue() + (1-fracneg)*white.getBlue());

    }
    
    return ret;
  };
  
  std::function<void(const string&)> helper =
    [&helper,&color_choice_ratio, &compare_tree] (const string& root) -> void {

    VertexData vd = compare_tree->getVertexData(root);
    double ratio = ((double)vd.cnt1)/(vd.cnt2>0?vd.cnt2:0.00000001); //avoiding NaNs

    if (vd.cnt1 != 0 || vd.cnt2 != 0)
      compare_tree->getVertex(root)->setColor(color_choice_ratio(ratio, 5));
    else
      compare_tree->getVertex(root)->setColor(Color("white"));
    
    for (auto & e : compare_tree->outgoingEdgeSetOf(root)) {
      auto to = e.to();
      helper(to);
    }    
  };
  
  helper(root);
}


void resize_compare(GraphAdjList<string, VertexData> * compare_tree,  const string& root) {

  std::map<int, int> maxsize = max_cnt12_per_level_rec(compare_tree, root, 0);
  
  std::function<void(GraphAdjList<string, VertexData> *, const string&, int)> helper =
    [&helper, &maxsize] (GraphAdjList<string, VertexData> * compare_tree, const string& root, int depth) -> void {

    VertexData vd = compare_tree->getVertexData(root);

    if (maxsize[depth] > 0) {
      int cnt = std::max(vd.cnt1, vd.cnt2);
      //std::cerr<<"depth: "<<depth<<" cnt: "<<cnt<<" maxsize: "<<maxsize[depth]<<"\n";
      compare_tree->getVertex(root)->setSize(5+(45.*cnt)/maxsize[depth]);
    }
    
    for (auto & e : compare_tree->outgoingEdgeSetOf(root)) {
      auto to = e.to();
      helper(compare_tree, to, depth+1);
    }    
  };
  
  helper(compare_tree, root, 0);
}
