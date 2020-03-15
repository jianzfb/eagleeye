#include "eagleeye/algorithm/nms.h"
#include "eagleeye/basic/MatrixMath.h"

namespace eagleeye{
Matrix<float> wnms(Matrix<float> dets,
							   float thresh, 
							   float prob_thresh) {
	Matrix<float> x1 = dets(Range(0, dets.rows()), Range(0, 1));
	Matrix<float> y1 = dets(Range(0, dets.rows()), Range(1, 2));
	Matrix<float> x2 = dets(Range(0, dets.rows()), Range(2, 3));
	Matrix<float> y2 = dets(Range(0, dets.rows()), Range(3, 4));
    Matrix<float> scores = dets(Range(0, dets.rows()), Range(4, 5));

	std::vector<unsigned int> order = sort<DescendingSort<float>>(scores);
	Matrix<float> areas = (x2 - x1 + eagleeye_eps).mul(y2 - y1 + eagleeye_eps);
	std::vector<unsigned int> keep;
	std::vector<Matrix<float>> result;
    while (order.size() > 0) {
		unsigned int index = order[0];
		if (scores.at(index) < prob_thresh) {
			break;
		}

		keep.push_back(index);
		if (order.size() == 1) {
			Matrix<float> weighted_bbox(1,5);
			weighted_bbox.at(0,0) = x1.at(index); weighted_bbox.at(0,1) = y1.at(index); weighted_bbox.at(0,2) = x2.at(index); weighted_bbox.at(0,3) = y2.at(index); weighted_bbox.at(0,4) = scores.at(index);
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
            Matrix<float> x1_weight = (x1.select(order, 1)).select(inds_weight_mask, 0);
            Matrix<float> y1_weight = (y1.select(order, 1)).select(inds_weight_mask, 0);
            Matrix<float> x2_weight = (x2.select(order, 1)).select(inds_weight_mask, 0);
            Matrix<float> y2_weight = (y2.select(order, 1)).select(inds_weight_mask, 0);
            Matrix<float> iou_weight = ovr.select(inds_weight_mask, 0);
			float iou_weight_sum = vsum(iou_weight);

			float x1_w = (vsum(x1_weight.mul_(iou_weight)) + x1.at(index))/(1 + iou_weight_sum);
            float y1_w = (vsum(y1_weight.mul_(iou_weight)) + y1.at(index))/(1 + iou_weight_sum);
			float x2_w = (vsum(x2_weight.mul_(iou_weight)) + x2.at(index))/(1 + iou_weight_sum);
			float y2_w = (vsum(y2_weight.mul_(iou_weight)) + y2.at(index))/(1 + iou_weight_sum);

			Matrix<float> weighted_bbox(1,5);
			weighted_bbox.at(0,0) = x1_w; weighted_bbox.at(0,1) = y1_w; weighted_bbox.at(0,2) = x2_w; weighted_bbox.at(0,3) = y2_w; weighted_bbox.at(0,4) = scores.at(index);
			result.push_back(weighted_bbox);
        }
        else{
			Matrix<float> weighted_bbox(1,5);
			weighted_bbox.at(0,0) = x1.at(index); weighted_bbox.at(0,1) = y1.at(index); weighted_bbox.at(0,2) = x2.at(index); weighted_bbox.at(0,3) = y2.at(index); weighted_bbox.at(0,4) = scores.at(index);
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

	Matrix<float> ss = concat(result, 0);
	return ss;
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