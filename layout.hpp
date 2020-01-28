#ifndef LAYOUT_H
#define LAYOUT_H

#include "classification.h"

void reset_tree(GraphAdjList<string, VertexData> * classification_tree);
  
void scale_intermediary(GraphAdjList<string, VertexData> * classification_tree,  const string& root);
void layout_basic(GraphAdjList<string, VertexData> * classification_tree,  const string& root);
void layout_radial_layered(GraphAdjList<string, VertexData> * classification_tree,  const string& root);

void color_compare(GraphAdjList<string, VertexData> * classification_tree,  const string& root);

void resize_compare(GraphAdjList<string, VertexData> * compare_tree,  const string& root);

#endif
