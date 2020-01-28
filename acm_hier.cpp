#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include "rapidjson/document.h"
#include "assert.h"
#include "rapidjson/error/en.h"

using namespace std;
using namespace rapidjson;

#include <curl/curl.h>

#include "Bridges.h"
#include <GraphAdjList.h>

using namespace bridges;

// parser to read acm classification data

struct VertexData {
	int hits;
	int cnt1, cnt2;
	string col;
	string parent;
	int depth = 0;

	VertexData() {
		VertexData(0, 0, 0, "blue", "", 0);	
	}
	VertexData(int h, int c1, int c2, string c, string par, int d) {
		hits = c1 = c2 = 0; c = "blue"; parent = par; depth = d;
	}
};

GraphAdjList<string, VertexData> *getACMClassificationHierarchy(string& root);
GraphAdjList<string, VertexData> *getClassificationHierarchy(string root, 
		vector<int> assignment_ids, GraphAdjList<string, VertexData> *acm_hier_graph);

string makeRequest(const string& url, const vector<string>& headers, 
							const string& data = "");

static size_t curlWriteFunction(void *contents, size_t size, size_t nmemb, void *results);
int main() {

	Bridges bridges(2, "kalpathi60", "486749122386");
	bridges.setServer("clone");
	string root = "";
	GraphAdjList<string, VertexData> *acm_hier = getACMClassificationHierarchy(root);
	cout << "Root: " + root << endl;
	vector<int> v  = {131,132};
	GraphAdjList<string, VertexData> *classif_graph = getClassificationHierarchy(root, v, acm_hier);

	bridges.setTitle("ACM Classification Hierarchy");
	bridges.setDataStructure(classif_graph);
	bridges.visualize();
}

GraphAdjList<string, VertexData> *getACMClassificationHierarchy(string& root_key) {

	string acm_hier_str = makeRequest (
			"http://unfrozen-materials-cs.herokuapp.com/static/assignments/js/ACM.json", 
			{"Accept: application/json"});

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


GraphAdjList<string, VertexData> *getClassificationHierarchy(string root, vector<int> assignment_ids, 
		GraphAdjList<string, VertexData> *acm_hier) {
	string ids = "";
	for (int k = 0; k < assignment_ids.size(); k++) 
		ids += std::to_string(assignment_ids[k]) + ",";	
	cout << endl << ids << endl;
	string classif_str = makeRequest (
			"http://unfrozen-materials-cs.herokuapp.com/data/?assignments=" + ids,
			{"Accept: application/json"});
//	cout << "Assignment 131:"  << endl << classif_str << endl;

	Document d;
	d.Parse (classif_str.c_str()); 

						// keep track of all classification strings for later use in
						// building the graph
	set<string> classif_set;
	const Value& assn_arr = d["assignments"];
	for (SizeType i = 0; i < assn_arr.Size(); i++) {   // iterate on assignments

								// iterate to find the "fields" tag
		for (Value::ConstMemberIterator itr = assn_arr[i].MemberBegin(); 
							itr != assn_arr[i].MemberEnd(); ++itr) {
			if (itr->name == "fields") {
				for (Value::ConstMemberIterator f_itr = itr->value.MemberBegin(); 
							f_itr != itr->value.MemberEnd(); ++f_itr) {
					if (f_itr->name == "classifications") {
						const Value& c_arr = f_itr->value;
						assert (c_arr.IsArray());
						for (SizeType j = 0; j < c_arr.Size(); j++) {
							string classif_str = c_arr[j].GetString();
							classif_set.emplace(classif_str);
							if (acm_hier->getVertices()->find(classif_str) == 
										acm_hier->getVertices()->end()) {
								cout << "classification not found!: " << classif_str << endl;
								continue;
							}
											 // update vertex data by incrementing hit count
							VertexData vd = acm_hier->getVertexData(classif_str);
							vd.hits++;
							acm_hier->setVertexData(classif_str, vd);
//							cout << acm_hier->getVertex(classif_str)->getLabel() << ": " << 
//									acm_hier->getVertexData(classif_str).hits << 
//									": " << acm_hier->getVertexData(classif_str).parent << 
//									": " << acm_hier->getVertexData(classif_str).depth << endl;
						}
					}
				}
			}
		}
	}

						// now the acm tree hierarchy contains the query nodes with hit count
						// construct a smaller tree containing those nodes + 1 more level
						// start at the root node

	cout << "num classifications:" << classif_set.size() << endl;
	GraphAdjList<string, VertexData> *classif_graph = new GraphAdjList<string, VertexData>;
						// mark array, initialize 
	unordered_map<string, bool> mark;
	for (auto& classif_str : classif_set) {
		mark[classif_str] = false;
	}
						// create the root node
	classif_graph->addVertex(root);
	classif_graph->getVisualizer(root)->setColor(Color("red"));
	mark[root] = true;
// Debug
/*
	for (auto& classif_str : classif_set) {
		string node = classif_str;
		bool done = false;
		while (!done) {
			cout << "Node:" << node << endl;
			vd = acm_hier->getVertexData(node);
			cout << "\t Parent:" << vd.parent << ", depth:" << vd.depth << endl;
			if (vd.depth == 0) done = true;
			else node = vd.parent;
		}
	}
	exit(0);
			
			
*/
// Debug
							// now add the edges using the mark array
	for (auto& classif_str : classif_set) {
		VertexData vd_node, vd_parent;
		string node = classif_str;
cout << "Node:" << node << endl;
							// create node vertex
		vd_node = acm_hier->getVertexData(classif_str);
		classif_graph->addVertex(classif_str, vd_node);
		classif_graph->getVisualizer(classif_str)->setColor(Color("orange"));
		while (!mark[node]) {
			mark[node] = true;
			vd_node = acm_hier->getVertexData(node);
cout << "\t Parent:" << vd_node.parent <<", depth:" << vd_node.depth << endl;
			if (!mark[vd_node.parent]) {
				vd_parent = acm_hier->getVertexData(vd_node.parent);
				classif_graph->addVertex(vd_node.parent, vd_parent);
				classif_graph->addEdge (vd_node.parent, node);
				cout << "Adding edge from\n\t\t" + vd_node.parent + "\n to \t\t" + node << endl;
				node = vd_node.parent;
			}
			else { // create the last link to parent which is already created
				classif_graph->addEdge (vd_node.parent, node);
			}
		}
	}
	
	return classif_graph;
}

