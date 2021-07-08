#include "eagleeye/algorithm/nms.h"
#include "eagleeye/basic/MatrixMath.h"

namespace eagleeye{
Matrix<float> wnms(Matrix<float> dets, float thresh, float prob_thresh) {
	int dim = dets.cols();
	int score_axis = dim - 1;

	Matrix<float> x1 = dets(Range::ALL(), Range(0, 1));
	Matrix<float> y1 = dets(Range::ALL(), Range(1, 2));
	Matrix<float> x2 = dets(Range::ALL(), Range(2, 3));
	Matrix<float> y2 = dets(Range::ALL(), Range(3, 4));
    Matrix<float> scores = dets(Range::ALL(), Range(score_axis, score_axis+1));

	std::vector<unsigned int> order = sort<DescendingSort<float>>(scores);
	Matrix<float> areas = (x2 - x1 + eagleeye_eps).mul(y2 - y1 + eagleeye_eps);
	std::vector<Matrix<float>> result;
    while (order.size() > 0) {
		unsigned int index = order[0];
		if (scores.at(index) < prob_thresh) {
			break;
		}
		if (order.size() == 1) {
			Matrix<float> weighted_bbox(1, dim);
			weighted_bbox.copy(dets(Range(index, index+1),Range::ALL()));
			result.push_back(weighted_bbox);
			break;
		}

		Matrix<float> xx1 = mmax(x1, x1.at(index), order, 1);
		Matrix<float> yy1 = mmax(y1, y1.at(index), order, 1);
		Matrix<float> xx2 = mmin(x2, x2.at(index), order, 1);
		Matrix<float> yy2 = mmin(y2, y2.at(index), order, 1);

		Matrix<float> w = mmax(xx2 - xx1 + eagleeye_eps, 0.0f);
		Matrix<float> h = mmax(yy2 - yy1 + eagleeye_eps, 0.0f);

		Matrix<float> inter = w.mul(h);
		Matrix<float> ovr = inter.div(areas.select(order, 1) + areas.at(index) - inter);
		Matrix<int> inds = boolean<gt<float>, int>(ovr, thresh);
        std::vector<unsigned int> inds_weight_mask;
        for (int m = 0; m < inds.rows(); ++m) {
            if (inds.at(m) > 0) {
                inds_weight_mask.push_back(m);
            }
        }
        if(inds_weight_mask.size() > 0){
			Matrix<float> around_dets = (dets.select(order, 1)).select(inds_weight_mask, 0);
			Matrix<float> best_and_around_coords = 
					concat(std::vector<Matrix<float>>{
							dets(Range(index, index+1),Range(0, dim-1)), 
							around_dets(Range::ALL(), Range(0, dim-1))}, 0);
			
			Matrix<float> best_and_around_score = 
					concat(std::vector<Matrix<float>>{
							dets(Range(index, index+1), Range(score_axis, score_axis+1)),
					 		around_dets(Range::ALL(), Range(score_axis, score_axis+1))}, 0);

			float total_score = vsum(best_and_around_score);	

			Matrix<float> weighted_best_and_around_coords = best_and_around_coords.mul(best_and_around_score);
			Matrix<float> sum_weighted_best_and_around_coords = msum(weighted_best_and_around_coords, 0);
			sum_weighted_best_and_around_coords.div_(total_score);

			Matrix<float> weighted_det(1, dim);
			weighted_det(Range(0,1), Range(0, dim-1)).copy(sum_weighted_best_and_around_coords);
			weighted_det.at(0, score_axis) = total_score/(inds_weight_mask.size()+1);
			result.push_back(weighted_det);
        }
        else{
			Matrix<float> weighted_bbox(1, dim);
			weighted_bbox.copy(dets(Range(index, index+1),Range::ALL()));
			result.push_back(weighted_bbox);
        }

		std::vector<unsigned int> remained_order;
		for (int m = 0; m < inds.rows(); ++m) {
			if (inds.at(m) == 0) {
				remained_order.push_back(order[m + 1]);
			}
		}

		order = remained_order;
	}

	Matrix<float> concated_result = concat(result, 0);
	return concated_result;
}


std::vector<unsigned int> nms(Matrix<float> dets, float thresh, float prob_thresh) {
	Matrix<float> x1 = dets(Range(0, dets.rows()), Range(0, 1));
	Matrix<float> y1 = dets(Range(0, dets.rows()), Range(1, 2));
	Matrix<float> x2 = dets(Range(0, dets.rows()), Range(2, 3));
	Matrix<float> y2 = dets(Range(0, dets.rows()), Range(3, 4));
	Matrix<float> scores = dets(Range(0, dets.rows()), Range(4, 5));

	std::vector<unsigned int> order = sort<DescendingSort<float>>(scores);
	Matrix<float> areas = (x2 - x1 + eagleeye_eps).mul(y2 - y1 + eagleeye_eps);
	std::vector<unsigned int> keep;

	while (order.size() > 0) {
		unsigned int index = order[0];
		if (scores.at(index) < prob_thresh) {
			break;
		}
		keep.push_back(index);
		if (order.size() == 1) {
			break;
		}

		Matrix<float> xx1 = mmax(x1, x1.at(index), order, 1);
		Matrix<float> yy1 = mmax(y1, y1.at(index), order, 1);
		Matrix<float> xx2 = mmin(x2, x2.at(index), order, 1);
		Matrix<float> yy2 = mmin(y2, y2.at(index), order, 1);

		Matrix<float> A = xx2.sub_(xx1).add_(eagleeye_eps);
		Matrix<float> B = yy2.sub_(yy1).add_(eagleeye_eps);
		Matrix<float> w = mmax_(A, 0.0f);
		Matrix<float> h = mmax_(B, 0.0f);

		Matrix<float> inter = w.mul_(h);
		Matrix<float> ovr = inter.div_(areas.select(order, 1).add_(areas.at(index)).sub_(inter));
		Matrix<int> inds = boolean<lt<float>, int>(ovr, thresh);
		int inds_num = inds.rows()*inds.cols();
		std::vector<unsigned int> remained_order;
		for (int m = 0; m < inds_num; ++m) {
			if (inds.at(m) > 0) {
				remained_order.push_back(order[m + 1]);
			}
		}

		order = remained_order;
	}

	return keep;
}
}