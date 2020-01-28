#include "classification.h"
#include "layout.hpp"

#define  ACM_TREE  0
#define  CLASSIFICATION_2214 0
#define  COMPARE  0
#define SIMILARITY 1

string replaceFirstOccurence(string stringIn, string pattern, string replaceWith) {
  auto ind = stringIn.find(pattern);
  if (ind == string::npos)
    return stringIn;
  else
    return stringIn.replace(ind, pattern.length(), replaceWith);
}

int main(int argc, char* argv[]) {

  if (argc < 3) {
    std::cerr<<"usage: "<<argv[0]<<" assignmentid username apikey"<<"\n";
    std::cerr<<"consider using \"make run\""<<"\n";
    return -1;
  }
  
  try {
  
    Bridges bridges(std::atoi(argv[1]), argv[2], argv[3]);

	bridges.setServer("clone");
	string root = "";

	VertexData vd;
							// parse and build the acm classification tree
	GraphAdjList<string, VertexData> *acm_tree = getACMClassificationTree(root);

	//layout_basic(acm_tree, root);
	layout_radial_layered(acm_tree, root);
	
	cout << "Root: " + root << endl;

	vector<int> jamie_soft = {165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177};

	vector<int> erik_2214 = {139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151,
				 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164};

	vector<int> erik_2214_lecture = {139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151,
					 152, 153};

	vector<int> erik_2214_assessment = {154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164};
	

	vector<int> kr_2214 = {120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,132, 
			       133, 134, 135, 136, 137, 138};

	vector<int> erik_3145 = {99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
				 111, 112, 113, 114, 115, 116, 117, 118, 119};

	vector<int> erik_3145_lecture = {108, 109, 110,
					 111, 112, 113, 114, 115, 116, 117, 118, 119};

	vector<int> erik_3145_assignment = {99, 100, 101, 102, 103, 104, 105, 106, 107, 108};


#if CLASSIFICATION_2214
	//vector<int> v  = {120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,132, 
	//						133, 134, 135, 136, 137, 138};

	vector<int> v = {139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164}; //erik's 2214
	
							// parse and build the queried classification tree
	GraphAdjList<string, VertexData> *classif_tree = getClassificationTree(root, v, acm_tree);

#elif COMPARE
	
	

	//vector<int>& v1 = erik_3145_lecture;
	//vector<int>& v2 = erik_3145_assignment;

	//vector<int>& v1 = erik_2214_lecture;
	//vector<int>& v2 = erik_2214_assessment;
	
	// vector<int>& v1 = erik_2214;
	// vector<int>& v2 = kr_2214;

	
	vector<int>& v1 = erik_2214;
	vector<int>& v2 = jamie_soft;
	
	GraphAdjList<string, VertexData> *erik_tree = getClassificationTree(root, v1, acm_tree);

	scale_intermediary(erik_tree, root);
	//layout_basic(erik_tree, root);
	layout_radial_layered(erik_tree, root);

	reset_tree(acm_tree);
	GraphAdjList<string, VertexData> *kr_tree = getClassificationTree(root, v2, acm_tree);

	scale_intermediary(kr_tree, root);
	//layout_basic(kr_tree, root);
	layout_radial_layered(kr_tree, root);

	
	GraphAdjList<string, VertexData> *classif_tree = compareClassifications(root,
							v1, v2, acm_tree);
	//layout_basic(classif_tree, root);
	layout_radial_layered(classif_tree, root);
	color_compare(classif_tree, root);
	resize_compare(classif_tree, root);	
#endif

#if SIMILARITY
	//std::vector<int>& v1 = erik_3145;
	//std::vector<int>& v2 = erik_2214;

	std::vector<int>& v1 = erik_2214_assessment;
	std::vector<int>& v2 = erik_2214_lecture;

	//std::vector<int>& v1 = erik_2214;
	//std::vector<int>& v2 = kr_2214;

	
	std::map<int, Assignment > vclass;

	
	GraphAdjList<int, Assignment> similarity_graph;
	  
	for (int id : v1) {
	  vclass[id] = getClassificationOf (id);

	  string title = vclass[id].title;
	  title = replaceFirstOccurence(title, "UNCC-ITCS-2214-Saule", "Y-DS");
	  title = replaceFirstOccurence(title, "UNCC-ITCS-2214-KRS", "X-DS");
	  
	  
	  std::cerr<<id<<" "<<vclass[id].title<<"\n";
	  similarity_graph.addVertex(id, vclass[id]);
	  similarity_graph.getVertex(id)->setLabel(title);
	  similarity_graph.getVertex(id)->setShape(CIRCLE);
	}

	for (int id : v2) {
	  vclass[id] = getClassificationOf (id);

	  string title = vclass[id].title;
	  title = replaceFirstOccurence(title, "UNCC-ITCS-2214-KRS", "X-DS");
	  title = replaceFirstOccurence(title, "UNCC-ITCS-2214-Saule", "Y-DS");

	  
	  std::cerr<<id<<" "<<vclass[id].title<<"\n";
	  similarity_graph.addVertex(id, vclass[id]);
	  similarity_graph.getVertex(id)->setLabel(title);
	  similarity_graph.getVertex(id)->setColor(Color("red"));
	  similarity_graph.getVertex(id)->setShape(SQUARE);
	}

	
	
	for (int id1 : v1) {
	  std::cerr<<id1<<" : ";
	  for (int id2 : v2) {
	    if (id1 == id2) continue;
	    
	    double cosine_sim = cosine(vclass[id1].classification, vclass[id2].classification);

	    std::cerr<<cosine_sim<<" ";

	    if (cosine_sim > 0.3) {
	      similarity_graph.addEdge(id1, id2);
	    }
	  }
	  std::cerr<<"\n";
	}

	bridges.setDataStructure(similarity_graph);
	bridges.visualize();

#endif
	
#if ACM_TREE
	bridges.setTitle("ACM Classification Hierarchy");
	bridges.setDataStructure(acm_tree);
	bridges.visualize();
#elif CLASSIFICATION_2214 
	bridges.setTitle("Classification Tree(2214)");
	bridges.setDataStructure(classif_tree);
	bridges.visualize();
#elif COMPARE
	bridges.setTitle("Comparing Two Sets of Learning Material: kr, erik");
	bridges.setDataStructure(acm_tree);
	bridges.visualize();
	bridges.setDataStructure(erik_tree);
	bridges.visualize();
	bridges.setDataStructure(kr_tree);
	bridges.visualize();
	bridges.setDataStructure(classif_tree);
	bridges.visualize();
#endif


	// To Do: Need proper destructors for graphs!! Not sure if it has been debugged and tested!

  } catch (std::string s) {
    std::cerr<<"string exception caught: "<< s<<std::endl;
  } catch (rapidjson_exception rje) {
    std::cerr<<"rapidjson_exception Exception caught: "<< rje.why
	     <<" in "<<rje.filename<<" "<<rje.linenumber<<std::endl;
  }
}
