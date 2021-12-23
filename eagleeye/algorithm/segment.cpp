#include "eagleeye/algorithm/segment.h"
#include "eagleeye/algorithm/disjoint-set.h"

namespace eagleeye{
// threshold function
#define THRESHOLD(size, c) (c/size)

typedef struct {
	float w;
	int a, b;
} edge;

bool operator<(const edge &a, const edge &b) {
	return a.w < b.w;
}

/*
* Segment a graph
*
* Returns a disjoint-set forest representing the segmentation.
*
* nu_vertices: number of vertices in graph.
* nu_edges: number of edges in graph
* edges: array of edges.
* c: constant for treshold function.
*/
universe *segmentGraph(int nu_vertices, int nu_edges, edge *edges, float c) { 
	// sort edges by weight
	std::sort(edges, edges + nu_edges);

	// make a disjoint-set forest
	universe *u = new universe(nu_vertices);

	// init thresholds
	float *threshold = new float[nu_vertices];
	for (int i = 0; i < nu_vertices; i++)
		threshold[i] = THRESHOLD(1,c);

	// for each edge, in non-decreasing weight order...
	for (int i = 0; i < nu_edges; i++) {
		edge *pedge = &edges[i];

		// components conected by this edge
		int a = u->find(pedge->a);
		int b = u->find(pedge->b);
		if (a != b) {
			if ((pedge->w <= threshold[a]) &&
				(pedge->w <= threshold[b])) {
					u->join(a, b);
					a = u->find(a);
					threshold[a] = pedge->w + THRESHOLD(u->size(a), c);
			}
		}
	}

	// free up
	delete[] threshold;
	return u;
}    

int segmentImage(Matrix<Array<float,3>> &_src3f, Matrix<int> &pImgInd, float sigma, float c, int min_size)
{
	int width = _src3f.cols();
    int height = _src3f.rows();
	Matrix<Array<float,3>> smImg3f = _src3f;
	// GaussianBlur(_src3f, smImg3f, Size(), sigma, 0, BORDER_REPLICATE);
    
	// build graph
	edge *edges = new edge[width*height*4];
	int num = 0; 
	{
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				if (x < width-1) {
					edges[num].a = y * width + x;
					edges[num].b = y * width + (x+1);
					// edges[num].w = diff(smImg3f, x, y, x+1, y);
                    edges[num].w = smImg3f.at(x,y).dist(smImg3f.at(x+1,y));
					num++;
				}

				if (y < height-1) {
					edges[num].a = y * width + x;
					edges[num].b = (y+1) * width + x;
					// edges[num].w = diff(smImg3f, x, y, x, y+1);
                    edges[num].w = smImg3f.at(x,y).dist(smImg3f.at(x, y+1));
					num++;
				}

				if ((x < width-1) && (y < height-1)) {
					edges[num].a = y * width + x;
					edges[num].b = (y+1) * width + (x+1);
					// edges[num].w = diff(smImg3f, x, y, x+1, y+1);
                    edges[num].w = smImg3f.at(x,y).dist(smImg3f.at(x+1,y+1));
					num++;
				}

				if ((x < width-1) && (y > 0)) {
					edges[num].a = y * width + x;
					edges[num].b = (y-1) * width + (x+1);
					// edges[num].w = diff(smImg3f, x, y, x+1, y-1);
                    edges[num].w = smImg3f.at(x,y).dist(smImg3f.at(x+1, y-1));
					num++;
				}
			}
		}
	}
	// segment
	universe *u = segmentGraph(width*height, num, edges, (float)c);

	// post process small components
	for (int i = 0; i < num; i++) {
		int a = u->find(edges[i].a);
		int b = u->find(edges[i].b);
		if ((a != b) && ((u->size(a) < min_size) || (u->size(b) < min_size)))
			u->join(a, b);
	}
	delete[] edges;

	// pick random colors for each component
	std::map<int, int> marker;
	// pImgInd.create(smImg3f.size(), CV_32S);
    pImgInd = Matrix<int>(smImg3f.rows(), smImg3f.cols());

	int idxNum = 0;
	for (int y = 0; y < height; y++) {
		// int *imgIdx = pImgInd.ptr<int>(y);
        int* imgIdx = pImgInd.row(y);
		for (int x = 0; x < width; x++) {
			int comp = u->find(y * width + x);
			if (marker.find(comp) == marker.end())
				marker[comp] = idxNum++;

			int idx = marker[comp];
			imgIdx[x] = idx;
		}
	}  
	delete u;
	return idxNum;
}


}