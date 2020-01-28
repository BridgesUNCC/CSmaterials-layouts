All the code start in main.cpp.

There are a few #define up that file that define what visualisation
you create. I believe only one of them should be at 1.

Setting ACM_TREE at 1 displays the ACM classification.

Setting CLASSIFICATION_2214 should build the tree of Erik's 2214.

Setting COMPARE at 1 creates a difference tree between Erik's 2214 and Jamie's capstone.

Setting SIMILARITY at 1 will build a graph of materials in Erik's 2214
and find assessmnet that are similar to lectures, this is done by
building a graph where two material are connected based on the cosine
similarity of their vector representation.

Overall, it is a bridges code that hits the CSMaterial API to get the
assignments. The trees are actually stored as bridges graphs to enable
fancy layouts. But the algorithms rely on the fact that they are
actually trees. Beware of the way main.cpp is written, variable name
may be somewhat incoherent with intent.

The code gets the tree with getACMClassificationTree() and information
about assignments with getClassificationTree(). Both are in
classification.cpp

The layouting and styling algorithms are in layout.cpp. The simpler
algorithm is layout_basic. It is a classic tree algorithm to count
how many "active" node in the subtrees and split an angle between the
different subtrees based on the number of nodes down stream.

layout_radial_layered is a little bit more involved. Essentially the
difference is that all siblings nodes get split in multiple layers so
as to minimize the likelihood they overlap each other.