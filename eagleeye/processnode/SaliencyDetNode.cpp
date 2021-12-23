#include "eagleeye/processnode/SaliencyDetNode.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/algorithm/segment.h"
// #include <opencv2/core/core.hpp>
// #include <opencv2/video/tracking.hpp>
// #include <opencv2/core/core.hpp>
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgproc/imgproc.hpp>

namespace eagleeye{
const int SaliencyDetNode::DefaultNums[3] = {12, 12, 12};
SaliencyDetNode::SaliencyDetNode(){
	 // 设置输出端口（拥有2个输出端口）
    this->setNumberOfOutputSignals(2);
    this->setOutputPort(new OutputPort_SALIENCY_Type, OUTPUT_PORT_SALIENCY);
	this->setOutputPort(new OutputPort_BOX_Type, OUTPUT_PORT_BOX);

	// 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

	this->m_segsigma = 1.0f;
	this->m_segk = 0.6f;
	this->m_segminsize = 5;
	this->m_sigmadist = 5.0f;
	this->m_saliency_thres = 0.6;

	EAGLEEYE_MONITOR_VAR(float,setSegSigma,getSegSigma,"segsigma","0.5", "10.0");
	EAGLEEYE_MONITOR_VAR(float,setSegK,getSegK,"segk","0.5","10.0");
	EAGLEEYE_MONITOR_VAR(int,setSegMinSize,getSegMinSize,"segms","5","50");
	EAGLEEYE_MONITOR_VAR(float, setSigmaDist,getSigmaDist,"sigmad","1.0","20.0");
	EAGLEEYE_MONITOR_VAR(float, setSaliencyThres,getSaliencyThres,"sathres","0.4","1.0");
}

SaliencyDetNode::~SaliencyDetNode(){

}

void SaliencyDetNode::executeNodeInfo(){
	// 1.step get input/output signal
	ImageSignal<Array<unsigned char, 3>>* input_img_sig = dynamic_cast<ImageSignal<Array<unsigned char, 3>>*>(this->m_input_signals[0]);
	ImageSignal<float>* saliency_sig = dynamic_cast<ImageSignal<float>*>(this->getOutputPort(OUTPUT_PORT_SALIENCY));
	ImageSignal<int>* saliency_box_sig = dynamic_cast<ImageSignal<int>*>(this->getOutputPort(OUTPUT_PORT_BOX));

	Matrix<Array<unsigned char, 3>> img = input_img_sig->getData();
	EAGLEEYE_LOGD("saliency rows %d cols %d", img.rows(), img.cols());

	// 2.step get saliency map
	Matrix<float> saliency = this->getRC(img);
	saliency_sig->setData(saliency);
	EAGLEEYE_LOGD("finish RC");
	// 3.step get saliency box
	int rows = saliency.rows(); int cols = saliency.cols();
	Matrix<int> bbox(1,4);
	int x1 = cols; int y1 = rows; int x2 = 0; int y2 = 0;
	for(int i=0; i<rows; ++i){
		float* saliency_ptr = saliency.row(i);
		for(int j=0; j<cols; ++j){
			if(saliency_ptr[j] > this->m_saliency_thres){
				if(x1 > j){
					x1 = j;
				}
				if(y1 > i){
					y1 = i;
				}
				if(x2 < j){
					x2 = j;
				}
				if(y2 < i){
					y2 = i;
				}
			}
		}
	}
	bbox.at(0,0) = x1;
	bbox.at(0,1) = y1;
	bbox.at(0,2) = x2-x1;
	bbox.at(0,3) = y2-y1;
	EAGLEEYE_LOGD("saliency bbox %d %d %d %d",x1,y1,x2-x1,y2-y1);
	saliency_box_sig->setData(bbox);
}

Matrix<float> SaliencyDetNode::getRC(Matrix<Array<unsigned char,3>> img){
	// cv::imwrite("/sdcard/input.png", cv::Mat(img.rows(),img.cols(),CV_8UC3, img.dataptr()));
	Matrix<Array<float,3>> imgLab3f = img.transform(RGB2LabOp<Array<unsigned char,3>>());
	Matrix<int> regIdx1i;
	int regNum = segmentImage(imgLab3f, regIdx1i, this->m_segsigma, this->m_segk, this->m_segminsize);	
	EAGLEEYE_LOGD("after segment");

	// Mat colorIdx1i, regSal1v, tmp, color3fv;
	Matrix<int> colorIdx1i;
	Matrix<float> regSal1v;
	Matrix<Array<float,3>> color3fv;
	Matrix<int> tmp;

	Matrix<Array<float,3>> img3f = img.transform<Array<float,3>>();
	int QuatizeNum = quantize(img3f, colorIdx1i, color3fv, tmp);
	EAGLEEYE_LOGD("quantize number %d",QuatizeNum);
	EAGLEEYE_LOGD("color3fv %d %d", color3fv.rows(),color3fv.cols());
	if(QuatizeNum <= 2){
		return Matrix<float>(img3f.rows(), img3f.cols());
	}

	color3fv = color3fv.transform(RGB2LabOp<Array<float, 3>>());
	std::vector<Region> regs(regNum);
	// regIdx1i is region label image
	// regs region statistic info
	// colorIdx1i quantized color index of every position
	buildRegions(regIdx1i, regs, colorIdx1i, color3fv.cols());
	regionContrast(regs, color3fv, regSal1v, this->m_sigmadist);
	regSal1v = regSal1v.transform(NormalizeMINMAX<float,float>(0.0f,1.0f));

	Matrix<float> sal1f(img3f.rows(), img3f.cols());
	float* regSal = (float*)regSal1v.dataptr();
	for (int r = 0; r < img3f.rows(); r++){
		const int* regIdx = regIdx1i.row(r);
		float* sal = sal1f.row(r);
		for (int c = 0; c < img3f.cols(); c++)
			sal[c] = regSal[regIdx[c]];
	}
	smoothByHist(img3f, colorIdx1i, color3fv, tmp, sal1f, 0.1f); 

	Matrix<unsigned char> dd = sal1f.transform(NormalizeMINMAX<float,unsigned char>(0,255));
	// cv::Mat sal1f_img(sal1f.rows(), sal1f.cols(), CV_8U, dd.dataptr());
	// cv::imwrite("/sdcard/sal.png", sal1f_img);

	smoothByRegion(sal1f, regIdx1i, regNum); 

	Matrix<unsigned char> cc = sal1f.transform(NormalizeMINMAX<float,unsigned char>(0,255));
	// cv::Mat sal1f_img_cc(sal1f.rows(), sal1f.cols(), CV_8U, cc.dataptr());
	// cv::imwrite("/sdcard/sal2.png", sal1f_img_cc);

    return sal1f;
}

Matrix<float> SaliencyDetNode::getHC(){
    return Matrix<float>();
}

int SaliencyDetNode::quantize(Matrix<Array<float,3>>& img3f, 
                                Matrix<int> &idx1i,
                                Matrix<Array<float,3>> &_color3f, 
                                Matrix<int> &_colorNum, 
                                float ratio, 
                                const int clrNums[3]){
	float clrTmp[3] = {clrNums[0] - 0.0001f, clrNums[1] - 0.0001f, clrNums[2] - 0.0001f};
	int w[3] = {clrNums[1] * clrNums[2], clrNums[2], 1};

    idx1i = Matrix<int>(img3f.rows(), img3f.cols());
	int rows = img3f.rows(), cols = img3f.cols();
	// Build color pallet
	std::map<int, int> pallet;
	for (int y = 0; y < rows; y++){
		const float* imgData = (float*)img3f.row(y);
		int* idx = idx1i.row(y);
		for (int x = 0; x < cols; x++, imgData += 3){
			idx[x] = (int)(imgData[0]*clrTmp[0])*w[0] + (int)(imgData[1]*clrTmp[1])*w[1] + (int)(imgData[2]*clrTmp[2]);
			pallet[idx[x]] ++;
		}
	}

	// Find significant colors
	int maxNum = 0;
	{
		int count = 0;
		std::vector<std::pair<int, int>> num; // (num, color) pairs in num
		num.reserve(pallet.size());
		for (std::map<int, int>::iterator it = pallet.begin(); it != pallet.end(); it++)
			num.push_back(std::pair<int, int>(it->second, it->first)); // (color, num) pairs in pallet
		sort(num.begin(), num.end(), std::greater<std::pair<int, int>>());

		maxNum = (int)num.size();
		int maxDropNum = int(rows * cols * (1-ratio) + 0.5f);
		for (int crnt = num[maxNum-1].first; crnt < maxDropNum && maxNum > 1; maxNum--)
			crnt += num[maxNum - 2].first;
		maxNum = eagleeye_min(maxNum, 256); // To avoid very rarely case
		if (maxNum <= 10)
			maxNum = eagleeye_min(10, (int)num.size());

		pallet.clear();
		for (int i = 0; i < maxNum; i++)
			pallet[num[i].second] = i; 

		std::vector<Array<int,3>> color3i(num.size());
		for (unsigned int i = 0; i < num.size(); i++){
			color3i[i][0] = num[i].second / w[0];
			color3i[i][1] = num[i].second % w[0] / w[1];
			color3i[i][2] = num[i].second % w[1];
		}

		// 找到丢弃的颜色量化
		for (unsigned int i = maxNum; i < num.size(); i++){
			int simIdx = 0, simVal = INT_MAX;
			for (int j = 0; j < maxNum; j++){
				int d_ij = color3i[i].sqrdist(color3i[j]);
				if (d_ij < simVal)
					simVal = d_ij, simIdx = j;
			}
			pallet[num[i].second] = pallet[num[simIdx].second];
		}
	}

    _color3f = Matrix<Array<float,3>>(1, maxNum);
    _colorNum = Matrix<int>(_color3f.rows(), _color3f.cols());

	Array<float,3>* color = (Array<float,3>*)(_color3f.dataptr());
	int* colorNum = (int*)(_colorNum.dataptr());
	for (int y = 0; y < rows; y++) {
        const Array<float,3>* imgData = img3f.row(y);
        int* idx = idx1i.row(y);
		for (int x = 0; x < cols; x++){
			idx[x] = pallet[idx[x]];
			color[idx[x]] += imgData[x];
			colorNum[idx[x]] ++;
		}
	}
	for (int i = 0; i < _color3f.cols(); i++)
		color[i] /= (float)colorNum[i];

	return _color3f.cols();
}

void SaliencyDetNode::buildRegions(Matrix<int>& regIdx1i, 
									std::vector<Region> &regs, 
									Matrix<int> &colorIdx1i, 
									int colorNum){
	int rows = regIdx1i.rows(), cols = regIdx1i.cols(), regNum = (int)regs.size();
	double cx = cols/2.0, cy = rows / 2.0;
	// Mat_<int> regColorFre1i = Mat_<int>::zeros(regNum, colorNum); // region color frequency
	Matrix<int> regColorFre1i(regNum, colorNum);
	for (int y = 0; y < rows; y++){
		const int *regIdx = regIdx1i.row(y);
		const int *colorIdx = colorIdx1i.row(y);
		for (int x = 0; x < cols; x++, regIdx++, colorIdx++){
			Region &reg = regs[*regIdx];
			reg.pixNum ++;
			reg.centroid[0] += x;
			reg.centroid[1] += y;
			regColorFre1i(*regIdx, *colorIdx)++;
			// reg.ad2c += Point2d(abs(x - cx), abs(y - cy));
			reg.ad2c[0] += abs(x - cx);
			reg.ad2c[1] += abs(y - cy);
		}
	}

	for (int i = 0; i < regNum; i++){
		Region &reg = regs[i];
		reg.centroid[0] /= reg.pixNum * cols;
		reg.centroid[1] /= reg.pixNum * rows;
		reg.ad2c[0] /= reg.pixNum * cols;
		reg.ad2c[1] /= reg.pixNum * rows;		
		int *regColorFre = regColorFre1i.row(i);
		for (int j = 0; j < colorNum; j++){
			float fre = (float)regColorFre[j]/(float)reg.pixNum;
			if (regColorFre[j] > eagleeye_eps)
				reg.freIdx.push_back(std::make_pair(fre, j));
		}
	}
}

template<typename T> inline T sqr(T x) { return x * x; } // out of range risk for T = byte, ...
void SaliencyDetNode::regionContrast(const std::vector<SaliencyDetNode::Region> &regs, 
									 Matrix<Array<float,3>> &color3fv, 
									 Matrix<float>& regSal1d, 
									 float sigmaDist){	
	// 记录颜色之间的距离
	Matrix<float> cDistCache1f(color3fv.cols(), color3fv.cols());
	{
		Array<float,3>* pColor = (Array<float,3>*)color3fv.dataptr();
		for(int i = 0; i < cDistCache1f.rows(); i++)
			for(int j= i+1; j < cDistCache1f.cols(); j++){
				cDistCache1f.at(i,j) = pColor[i].dist(pColor[j]);
				cDistCache1f.at(j,i) = cDistCache1f.at(i,j);
			}
	}

	int regNum = (int)regs.size();
	Matrix<float> rDistCache1d(regNum, regNum);
	regSal1d = Matrix<float>(1, regNum);
	float* regSal = (float*)regSal1d.dataptr();
	for (int i = 0; i < regNum; i++){
		const Array<float,2> &rc = regs[i].centroid;
		for (int j = 0; j < regNum; j++){
			if(i<j) {
				float dd = 0.0f;
				const std::vector<CostfIdx> &c1 = regs[i].freIdx, &c2 = regs[j].freIdx;
				for (size_t m = 0; m < c1.size(); m++)
					for (size_t n = 0; n < c2.size(); n++)
						dd += cDistCache1f.at(c1[m].second,c2[n].second) * c1[m].first * c2[n].first;
				rDistCache1d.at(j,i) = rDistCache1d.at(i,j) = dd * exp(-rc.sqrdist(regs[j].centroid)/sigmaDist); 
			}
			regSal[i] += regs[j].pixNum * rDistCache1d.at(i,j);
		}
		regSal[i] *= exp(-9.0 * (sqr(regs[i].ad2c[0]) + sqr(regs[i].ad2c[1])));
	}
}

void SaliencyDetNode::smoothByHist(Matrix<Array<float,3>> &img3f, 
									Matrix<int>& idx1i, 
									Matrix<Array<float,3>>& binColor3f, 
									Matrix<int>& colorNums1i, 
									Matrix<float> &sal1f, 
									float delta){
	int binN = binColor3f.cols();

	// Get initial color saliency
	Matrix<float> _colorSal(1, binN);
	int rows = img3f.rows(), cols = img3f.cols();{
		float* colorSal = (float*)_colorSal.dataptr();
		if (img3f.isfull() && sal1f.isfull())
			cols *= img3f.rows(), rows = 1;

		for (int y = 0; y < rows; y++){
			const int* idx = idx1i.row(y);
			const float* initialS = sal1f.row(y);
			for (int x = 0; x < cols; x++)
				colorSal[idx[x]] += initialS[x];
		}
		const int *colorNum = (int*)(colorNums1i.dataptr());
		for (int i = 0; i < binN; i++)
			colorSal[i] /= colorNum[i];

		_colorSal = _colorSal.transform(NormalizeMINMAX<float,float>(0.0f,1.0f));
	}

	// Find similar colors & Smooth saliency value for color bins
	std::vector<std::vector<CostfIdx>> similar(binN); // Similar color: how similar and their index
	Array<float,3>* color = (Array<float,3>*)binColor3f.dataptr();
	for (int i = 0; i < binN; i++){
		std::vector<CostfIdx> &similari = similar[i];
		similari.push_back(std::make_pair(0.f, i));
		for (int j = 0; j < binN; j++)
			if (i != j)
				similari.push_back(std::make_pair(color[i].dist(color[j]), j));
		sort(similari.begin(), similari.end());
	}
	smoothSaliency(colorNums1i, _colorSal, delta, similar);

	// Reassign pixel saliency values
	float* colorSal = (float*)(_colorSal.dataptr());
	for (int y = 0; y < rows; y++){
		const int* idx = idx1i.row(y);
		float* resSal = sal1f.row(y);
		for (int x = 0; x < cols; x++)
			resSal[x] = colorSal[idx[x]];
	}
}

void SaliencyDetNode::smoothSaliency(Matrix<int> &colorNum1i, 
									Matrix<float> &sal1f, 
									float delta, 
									const std::vector<std::vector<SaliencyDetNode::CostfIdx>> &similar){
	if (sal1f.cols() < 2)
		return;

	int binN = sal1f.cols();
	// Mat newSal1d= Mat::zeros(1, binN, CV_64FC1);
	Matrix<float> newSal1d(1,binN);
	float *sal = (float*)(sal1f.dataptr());
	float *newSal = (float*)(newSal1d.dataptr());
	int *pW = (int*)(colorNum1i.dataptr());

	// Distance based smooth
	int n = eagleeye_max((binN * delta),2);
	std::vector<float> dist(n);
	std::vector<float> val(n);
	std::vector<float> w(n);

	for (int i = 0; i < binN; i++){
		const std::vector<SaliencyDetNode::CostfIdx> &similari = similar[i];
		double totalDist = 0, totoalWeight = 0;
		for (int j = 0; j < n; j++){
			int ithIdx =similari[j].second;
			dist[j] = similari[j].first;
			val[j] = sal[ithIdx];
			w[j] = pW[ithIdx];
			totalDist += dist[j];
			totoalWeight += w[j];
		}
		double valCrnt = 0;
		for (int j = 0; j < n; j++)
			valCrnt += val[j] * (totalDist - dist[j]) * w[j];

		newSal[i] =  valCrnt / (totalDist * totoalWeight);
	}
	sal1f = newSal1d.transform(NormalizeMINMAX<float,float>(0.0f,1.0f));
}

void SaliencyDetNode::smoothByRegion(Matrix<float> &sal1f, Matrix<int> &segIdx1i, int regNum)
{
	std::vector<float> saliecy(regNum, 0);
	std::vector<int> counter(regNum, 0);
	for (int y = 0; y < sal1f.rows(); y++){
		const int *idx = segIdx1i.row(y);
		float *sal = sal1f.row(y);
		for (int x = 0; x < sal1f.cols(); x++){
			saliecy[idx[x]] += sal[x];
			counter[idx[x]] ++;
		}
	}

	for (size_t i = 0; i < counter.size(); i++)
		saliecy[i] /= counter[i];
	// Mat rSal(1, regNum, CV_64FC1, &saliecy[0]);
	Matrix<float> rSal(1, regNum, &saliecy[0]);
	rSal = rSal.transform(NormalizeMINMAX<float,float>(0.0f,1.0f));

	for (int y = 0; y < sal1f.rows(); y++){
		const int *idx = segIdx1i.row(y);
		float *sal = sal1f.row(y);
		for (int x = 0; x < sal1f.cols(); x++)
			sal[x] = (float)saliecy[idx[x]];
	}	
}

void SaliencyDetNode::setSegSigma(float v){
	m_segsigma = v;
	modified();
}
void SaliencyDetNode::getSegSigma(float& v){
	v = m_segsigma;
}

void SaliencyDetNode::setSegK(float v){
	m_segk = v;
	modified();
}
void SaliencyDetNode::getSegK(float& v){
	v = m_segk;
}

void SaliencyDetNode::setSegMinSize(int v){
	m_segminsize = v;
	modified();
}
void SaliencyDetNode::getSegMinSize(int& v){
	v = m_segminsize;
}
void SaliencyDetNode::setSigmaDist(float v){
	m_sigmadist = v;
	modified();
}
void SaliencyDetNode::getSigmaDist(float& v){
	v = m_sigmadist;
}

void SaliencyDetNode::setSaliencyThres(float v){
	this->m_saliency_thres = v;
	modified();
}
void SaliencyDetNode::getSaliencyThres(float& v){
	v = this->m_saliency_thres;
}
}