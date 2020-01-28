#include "classification.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include <curl/curl.h>
#include "rapidjson/document.h"
#include "assert.h"
#include "rapidjson/error/en.h"

using namespace std;
using namespace rapidjson;


// parser to read acm classification data


string makeRequest(const string& url, const vector<string>& headers, 
							const string& data = "");

static size_t curlWriteFunction(void *contents, size_t size, size_t nmemb, void *results);

//
//   This function parses and creates the ACM classification hierarchy (tree)
//
GraphAdjList<string, VertexData> *getACMClassificationTree(string& root_key) {

	string acm_hier_str = makeRequest (
			"http://unfrozen-materials-cs.herokuapp.com/static/assignments/js/ACM.json", 
			{"Accept: application/json"});

 std:cerr<<acm_hier_str<<"\n";
	
	GraphAdjList<string, VertexData> *graph = new GraphAdjList<string, VertexData>;
	Document acm_hier;
	acm_hier.Parse (acm_hier_str.c_str());

	for (Value::ConstMemberIterator itr = acm_hier.MemberBegin(); itr != acm_hier.MemberEnd(); 
													++itr) {
		Value::ConstMemberIterator value_itr;
		string name, node_val, parent_val;
		int depth;
		VertexData vd;
		for (value_itr = itr->value.MemberBegin(); value_itr != itr->value.MemberEnd();
										++value_itr) {
			name = value_itr->name.GetString();
			if (name == "id") { 		// check if this id exists
				node_val = value_itr->value.GetString();
				if (graph->getVertices()->find(node_val) == graph->getVertices()->end()) {
					graph->addVertex(node_val, vd);
				}
			}
			if (name == "parent") {	// check if parent exists
				parent_val = value_itr->value.GetString();
				if (graph->getVertices()->find(parent_val) == graph->getVertices()->end()) {
					graph->addVertex(parent_val, vd);
				}
			}
			if (name == "depth") {  // keep track of depth 
				depth = value_itr->value.GetInt();
			}
		}
						// link parent to node
		if (depth == 0) {// root
			root_key = node_val;
			graph->getVertex(node_val)->getVisualizer()->setColor(Color("red"));
			graph->getVertex(node_val)->getVisualizer()->setSize(50.0);
		}
		else {
			graph->addEdge(parent_val, node_val);
			vd.parent = parent_val;
			vd.depth = depth;
			graph->setVertexData(node_val, vd);
		}
	}
	return graph;

}
//
//
// Gets the classification tree based on a query of learning materials
//
// This is done in 3 parts:
//		1. Parse the topics/outcomes and mark it in the ACM tree
//		2. Construct classification tree of just these tags and follow the path to the root.
//		3. Add the immediate children of these nodes
//

set<string>  parseClassification(string root, vector<int> assignment_ids,
		GraphAdjList<string, VertexData> *acm_hier, int *max_hits);
GraphAdjList<string, VertexData> *buildClassificationTree (string root, 
		unordered_map<string, bool>& mark, set<string>& classif_set, 
							   GraphAdjList<string, VertexData>* acm_hier, int max_hits, bool copy_loc);
void addChildren(GraphAdjList<string, VertexData>* classification_tree, 
		 GraphAdjList<string, VertexData>* acm_tree, unordered_map<string, bool> mark, bool copy_loc);

GraphAdjList<string, VertexData> *getClassificationTree(
				string root, 
				vector<int> assignment_ids, 
				GraphAdjList<string, VertexData> *acm_tree,
				bool copy_loc) {
	int max_hits = 0;
	set<string>  classif_set = parseClassification (root, assignment_ids, acm_tree, &max_hits);

	unordered_map<string, bool>  mark;
	GraphAdjList<string, VertexData> *classification_tree = 
	  buildClassificationTree (root, mark, classif_set, acm_tree, max_hits, copy_loc);

	addChildren(classification_tree, acm_tree, mark, copy_loc);

	return classification_tree;
}


Assignment getClassificationOf(int assignment_id) {
  string classif_json = makeRequest (
				     "http://unfrozen-materials-cs.herokuapp.com/data/?assignments=" + std::to_string(assignment_id),
				    {"Accept: application/json"});

  //std::cerr<<"JSON: "<<classif_json<<std::endl;
  
  Document d;
  d.Parse (classif_json.c_str()); 

  Assignment ret;
  
  set<string> & classif_set = ret.classification;
  const Value & that_assignment =  d["assignments"][0];

  ret.title = that_assignment["fields"]["title"].GetString();
  
  auto const & classif_array = that_assignment["fields"]["classifications"].GetArray();

  for (int i=0; i<classif_array.Size(); ++i) {
    std::string  classif_entry = classif_array[i].GetString();
    classif_set.emplace(classif_entry);
  }
  
  return ret;
}

double cosine(const set<string>& s1, const set<string>& s2) {
  //sets are binary vector so, the cosine similarity is number of common entry/(||s1||||s2||)
  double count;
  for (const string &str : s1) {
    if (s2.find(str) != s2.end())
      count += 1.;
  }
  count /= std::sqrt(s1.size())*std::sqrt(s2.size());

  return count;
}

set<string>  parseClassification(
					string root, 
					vector<int> assignment_ids, 
					GraphAdjList<string, VertexData> *acm_hier, 
					int *max_hits) {

	string ids = "";
	for (int k = 0; k < assignment_ids.size(); k++) 
		ids += std::to_string(assignment_ids[k]) + ",";	
	cout << endl << ids << endl;
	string classif_str = makeRequest (
			"http://unfrozen-materials-cs.herokuapp.com/data/?assignments=" + ids,
			{"Accept: application/json"});

	Document d;
	d.Parse (classif_str.c_str()); 

						// keep track of all classification strings for later use in
						// building the graph
	set<string> classif_set;
	const Value& assn_arr = d["assignments"];
	*max_hits = 0;
	for (SizeType i = 0; i < assn_arr.Size(); i++) {   // iterate on assignments

						// iterate to find the "fields" tag
		for (Value::ConstMemberIterator itr = assn_arr[i].MemberBegin(); 
							itr != assn_arr[i].MemberEnd(); ++itr) {
			if (itr->name == "fields") {
			  std::string assignment_name = (itr->value)["title"].GetString();
				for (Value::ConstMemberIterator f_itr = itr->value.MemberBegin(); 
							f_itr != itr->value.MemberEnd(); ++f_itr) {
						// parse the classifications for this assignment id
						// update the acm hierarchy (vertex data) to keep track
						// of the hits and any counts  in the VertexData struct
					if (f_itr->name == "classifications") {
						const Value& c_arr = f_itr->value;
						assert (c_arr.IsArray());
						for (SizeType j = 0; j < c_arr.Size(); j++) {
							string classif_str = c_arr[j].GetString();

							if (acm_hier->getVertices()->find(classif_str) == 
										acm_hier->getVertices()->end()) {
								cout << "classification not found!: " << classif_str << endl;
								continue;
							}
							classif_set.emplace(classif_str);
							
											 // update vertex data by incrementing hit count
							VertexData vd = acm_hier->getVertexData(classif_str);
							vd.assignment_names.push_back(assignment_name);
							vd.hits++;
							*max_hits = (vd.hits > *max_hits) ? vd.hits : *max_hits;
							acm_hier->setVertexData(classif_str, vd);
						}
					}
				}
			}
		}
	}

	cout << "Max Hits: " << *max_hits << endl;

	return classif_set;
}
GraphAdjList<string, VertexData> *buildClassificationTree (
				string root, 
				unordered_map<string, bool>& mark,
				set<string>& classif_set, 
				GraphAdjList<string, VertexData>* acm_hier, 
				int max_hits,
				bool copy_loc) {

				// now the acm tree hierarchy contains the query nodes with hit count
				// construct a smaller tree containing those nodes and the path to the root

	GraphAdjList<string, VertexData> *classif_tree = new GraphAdjList<string, VertexData>;
						// mark array, initialize 
	for (auto& classif_str : classif_set) {
		mark[classif_str] = false;
	}
						// create the root node
	classif_tree->addVertex(root);
	if (copy_loc)
	  classif_tree->getVertex(root)->setLocation(acm_hier->getVertex(root)->getLocationX(),
						     acm_hier->getVertex(root)->getLocationY());

	classif_tree->getVisualizer(root)->setColor(Color("red"));
	classif_tree->getVisualizer(root)->setSize(50.0);
						// use a mark array to keep track of visited nodes
	mark[root] = true;
							// now add the edges using the mark array
	for (auto& classif_str : classif_set) {
		VertexData vd_node, vd_parent, vd_child;
		string node = classif_str;
							// create node vertex
		vd_node = acm_hier->getVertexData(classif_str);
		classif_tree->addVertex(classif_str, vd_node);

		if (copy_loc)
		  classif_tree->getVertex(classif_str)->setLocation(acm_hier->getVertex(classif_str)->getLocationX(),
								    acm_hier->getVertex(classif_str)->getLocationY());

							// mark the nodes from here to the root, to indicate the 
							// path of the selected node to the root
		while (!mark[node]) {
			mark[node] = true;
							// 	set attributes for this node
			vd_node = acm_hier->getVertexData(node);
							//  color orange if accessed, else blue
			classif_tree->getVisualizer(node)->setColor(Color(((vd_node.hits)? "orange": "blue")));
							// size the node based on the number of accesses
			classif_tree->getVisualizer(node)->setSize(((double)vd_node.hits/max_hits*40.0+10.0));
			string label = classif_tree->getVertex(node)->getLabel();
			label +=  "\n\t(Num Accesses: " + std::to_string(vd_node.hits) + ")";

			for (auto& assname: vd_node.assignment_names) {
			  label += "\n\t"+assname;
			}
			
			classif_tree->getVertex(node)->setLabel(label);

			if (!mark[vd_node.parent]) {	// follow the parent ptrs to the root
				vd_parent = acm_hier->getVertexData(vd_node.parent);
				classif_tree->addVertex(vd_node.parent, vd_parent);

				if (copy_loc)
				  classif_tree->getVertex(vd_node.parent)->setLocation(acm_hier->getVertex(vd_node.parent)->getLocationX(),
										       acm_hier->getVertex(vd_node.parent)->getLocationY());

				
				classif_tree->addEdge (vd_node.parent, node);
				//				classif_tree->getLinkVisualizer(vd_node.parent, node)->setThickness(6.0);
				//				classif_tree->getLinkVisualizer(vd_node.parent, node)->setColor(Color("orange"));
				node = vd_node.parent;
			}
			else { // create the last link to parent which is already created and marked
				classif_tree->addEdge (vd_node.parent, node);
			}
		}
	}
	
	return classif_tree;

}
void addChildren(
	GraphAdjList<string, VertexData>* classif_tree, 
	GraphAdjList<string, VertexData>* acm_tree, 
	unordered_map<string, bool> mark,
	bool copy_loc) {

					// iterate through the classified vertices to identify
					// immediate children and their edges
	vector<string> child_verts;
	for (auto v : *classif_tree->getVertices()) {
		for (auto edge: acm_tree->outgoingEdgeSetOf(v.first)) {
			string to = edge.to();
			if (!mark[to]) {
				child_verts.push_back (to);
				child_verts.push_back (v.first);
			}
		}
	}
	VertexData vd;
	for (int k = 0; k < child_verts.size(); k += 2) {
		classif_tree->addVertex(child_verts[k], vd);


		if (copy_loc)
		  classif_tree->getVertex(child_verts[k])->setLocation(acm_tree->getVertex(child_verts[k])->getLocationX(),
								       acm_tree->getVertex(child_verts[k])->getLocationY());

		
		classif_tree->addEdge(child_verts[k+1], child_verts[k]);
					// vertices are of default size, faded out and labeled 
		classif_tree->getVisualizer(child_verts[k])->setSize(10.0);
		classif_tree->getVisualizer(child_verts[k])->setOpacity(0.01);
		string label = classif_tree->getVertex(child_verts[k])->getLabel();
		classif_tree->getVertex(child_verts[k])->setLabel(label + "\n\t(Num Accesses: " + 
							std::to_string(vd.hits) + ")");
	}
}

void reset_tree (GraphAdjList<string, VertexData> *acmtree) {
  // reset the acm tree counts and assignment names
  for (auto v : *acmtree->getVertices()) {
    VertexData vd = acmtree->getVertexData(v.first);
    vd.hits = 0;
    vd.assignment_names.clear();
    acmtree->setVertexData(v.first, vd);
  }
}

/**
 *
 *   Compares two classification trees and creates a classification tree with 3 colored
 *   nodes: matched classifications(lightgrey), blue (present in 1, missing in 2), brown (missing in 1,
 *          present in 2
 *
 */
void compareClassificationsHelper ( string node, GraphAdjList<string, VertexData> *tree1, 
		GraphAdjList<string, VertexData> *tree2, GraphAdjList<string, 
		VertexData> *cmp_tree, GraphAdjList<string, VertexData> *acm_tree, 
		unordered_map<string, bool>& mark);

GraphAdjList<string, VertexData> *compareClassifications(
	string root,
	vector<int> ids1, 
	vector<int> ids2,
	GraphAdjList<string, VertexData> *acm_tree, bool copy_loc) {



  
	int max_hits1 = 0;
	int max_hits2 = 0;
	reset_tree(acm_tree);
	set<string>  classif_set1 = parseClassification (root, ids1, acm_tree, &max_hits1);


	unordered_map<string, bool>  mark;
    GraphAdjList<string, VertexData> *tree1 = 
      buildClassificationTree (root, mark, classif_set1, acm_tree, max_hits1, copy_loc);

								// update count l
	for (auto v : *tree1->getVertices()) {
	  VertexData vd;
	  vd = tree1->getVertexData(v.first);
		if (vd.hits) {
		  VertexData vdacm = acm_tree->getVertexData(v.first);
			vdacm.cnt1 = vd.hits;
			acm_tree->setVertexData(v.first, vdacm);
		}
	}
	

	
	reset_tree(acm_tree);
	set<string>  classif_set2 = parseClassification (root, ids2, acm_tree, &max_hits2);

	mark.clear();
    GraphAdjList<string, VertexData> *tree2 = 
      buildClassificationTree (root, mark, classif_set2, acm_tree, max_hits2, copy_loc);

								// update count 2
	for (auto v : *tree2->getVertices()) {
	  VertexData vd = tree2->getVertexData(v.first);
		if (vd.hits) {
		  VertexData vdacm = acm_tree->getVertexData(v.first);
		  vdacm.cnt2 = vd.hits;
			acm_tree->setVertexData(v.first, vdacm);
		}
	}
							// construct the final graph
	GraphAdjList<string, VertexData> *cmp_tree = new GraphAdjList<string, VertexData>;

	cmp_tree->addVertex(root);
	cmp_tree->getVisualizer(root)->setColor(Color("red"));
	cmp_tree->getVisualizer(root)->setSize(50.0);

	mark.clear();
	for (auto& v: *acm_tree->getVertices())
		mark[v.first] = false;

	mark[root] = true;

	for (auto& v : *acm_tree->getVertices()) {
	  VertexData vd = acm_tree->getVertexData(v.first);
		if ((vd.cnt1 || vd.cnt2) && !mark[v.first]) {
								//mark nodes from this vertex to the root
			compareClassificationsHelper(v.first, tree1, tree2, cmp_tree, acm_tree, mark);
		}
		//#define BINARY_COMPARE
#ifdef BINARY_COMPARE
		if (vd.cnt1 && vd.cnt2) {		// intersection
		  cmp_tree->getVisualizer(v.first)->setColor(Color("#DDDDDD"));
		}
		else if (vd.cnt1 && !vd.cnt2) {		// difference
		  cmp_tree->getVisualizer(v.first)->setColor(Color("blue"));
		}
		else if (!vd.cnt1 && vd.cnt2) { // difference
		  cmp_tree->getVisualizer(v.first)->setColor(Color("brown"));
		}
#endif
#ifndef BINARY_COMPARE
		if (vd.cnt1 || vd.cnt2) {
		  if (vd.cnt1 > 2.*vd.cnt2) //set 1 is bigger
		    cmp_tree->getVisualizer(v.first)->setColor(Color("blue"));
		  else if (vd.cnt2 > 2.*vd.cnt1) //set 2 is bigger
		    cmp_tree->getVisualizer(v.first)->setColor(Color("brown"));
		  else // same
		    cmp_tree->getVisualizer(v.first)->setColor(Color("#DDDDDD"));
		}
#endif

	}
	return cmp_tree;
}

void compareClassificationsHelper (string node, 
	GraphAdjList<string, VertexData> *classif_tree1,
	GraphAdjList<string, VertexData> *classif_tree2,
	GraphAdjList<string, VertexData> *cmp_tree,
	GraphAdjList<string, VertexData> *acm_tree,
	unordered_map<string, bool>& mark) {

	while (!mark[node]) {
		mark[node] = true;
		VertexData vd_node = acm_tree->getVertexData(node);
		cmp_tree->addVertex(node);
		cmp_tree->setVertexData(node, vd_node);
		{
		  std::string label = node;
		  if (classif_tree1->getVertex(node)) {
		    for (auto assname : classif_tree1->getVertexData(node).assignment_names)
		      label += "\n\t" + assname;
		  }

		  label += "\n\tVS";

		  if (classif_tree2->getVertex(node)) {
		    for (auto assname : classif_tree2->getVertexData(node).assignment_names)
		      label += "\n\t" + assname;
		  }
		  
		  cmp_tree->getVertex(node)->setLabel(label);
		}
		
		if (!mark[vd_node.parent]) {	// follow the parent ptrs to the root
			VertexData vd_parent = acm_tree->getVertexData(vd_node.parent);
			cmp_tree->addVertex(vd_node.parent, vd_parent);
			cmp_tree->addEdge (vd_node.parent, node);
			node = vd_node.parent;
		}
		else { // create the last link to parent which is already created
			cmp_tree->addEdge (vd_node.parent, node);
		}
	}
}



/**
	 * Uses Easy CURL library to execute a simple request.
	 *
	 * @param url The url destination for the request
	 * @param headers The headers for the request
	 * @param data The content sent in POST requests
	 * @throw string Thrown if curl request fails
*/
string makeRequest(const string& url, const vector<string>&
					headers, const string& data) {
	string results;
	// first load curl enviornment (only need be called
	// once in entire session tho)
	curl_global_init(CURL_GLOBAL_ALL);
	CURL* curl = curl_easy_init(); // get a curl handle
	if (curl) {
		char error_buffer[CURL_ERROR_SIZE];
		CURLcode res;
		//setting verbose
		if (0) {
			res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			if (res != CURLE_OK)
				throw "curl_easy_setopt failed";
		}
		// setting error buffer
		res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
		if (res != CURLE_OK)
			throw "curl_easy_setopt failed";

		// set the URL to GET from
		res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		if (res != CURLE_OK)
			throw "curl_easy_setopt failed";
		//pass pointer to callback function
		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &results);
		if (res != CURLE_OK)
			throw "curl_easy_setopt failed";
		//sends all data to this function
		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteFunction);
		if (res != CURLE_OK)
			throw "curl_easy_setopt failed";
		// need this to catch http errors
		res = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
		if (res != CURLE_OK)
			throw "curl_easy_setopt failed";
		if (data.length() > 0) {
			// Now specify the POST data
			res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
			if (res != CURLE_OK)
				throw "curl_easy_setopt failed";
			// Now specify the POST data size
			res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
			if (res != CURLE_OK)
				throw "curl_easy_setopt failed";
			//  a post request
			res = curl_easy_setopt(curl, CURLOPT_POST, 1L);
			if (res != CURLE_OK)
				throw "curl_easy_setopt failed";
		}

		struct curl_slist* curlHeaders = nullptr;
		for (const string& header : headers) {
			curlHeaders = curl_slist_append(curlHeaders, header.c_str());
		}
		res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);
		if (res != CURLE_OK)
			throw "curl_easy_setopt failed";

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			throw "curl_easy_perform() failed.\nCurl Error Code "
			+ to_string(res) + "\n" + curl_easy_strerror(res) +
			"\n"
			+ error_buffer + "\nPossibly Bad BRIDGES Credentials\n";
		}
		curl_slist_free_all(curlHeaders);
		curl_easy_cleanup(curl);
	}
	else {
		throw "curl_easy_init() failed!\nNothing retrieved from server.\n";
	}

	curl_global_cleanup();
	return results;
}


static size_t curlWriteFunction(void *contents, size_t size, size_t nmemb, void *results) {
	size_t handled = size * nmemb;
	if (results) {
		((string*)results)->append((char*)contents, handled);
	}
	return handled;
}

