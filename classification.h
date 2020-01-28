#ifndef CLASSIFICATION_H
#define CLASSIFICATION_H

#include "Bridges.h"
#include <GraphAdjList.h>
#include <string>
#include <set>

using namespace std;
using namespace bridges;
				// define vertex data type
struct VertexData {
	int hits = 0;
	int cnt1 = 0, cnt2 = 0;
	string col = "blue";
	string parent = "";
	int depth = 0;

  std::vector<std::string> assignment_names; 
  
	VertexData() {
	}
	VertexData(int h, int c1, int c2, string c, string par, int d) {
		hits = cnt1 = cnt2 = 0; col = "blue"; parent = par; depth = d;
	}
};
						// parser to read acm classification data

struct Assignment {
  std::string title;
  std::set<std::string> classification;
};

GraphAdjList<string, VertexData> *getACMClassificationTree(string& root);

GraphAdjList<string, VertexData> *getClassificationTree(string root, 
		vector<int> assignment_ids, GraphAdjList<string, VertexData> *acm_hier_graph, bool copy_loc=true);

/* GraphAdjList<string, VertexData> *getClassificationTree(string root,  */
/* 		vector<int> assignment_ids, GraphAdjList<string, VertexData> *acm_hier_graph, bool copy_loc=true); */

GraphAdjList<string, VertexData> *compareClassifications(string root, vector<int> ids1,
							 vector<int> ids2, GraphAdjList<string, VertexData> *acm_tree, bool copy_loc=true);


Assignment getClassificationOf(int assignment_id);
double cosine(const set<string>& s1, const set<string>& s2);

#endif
