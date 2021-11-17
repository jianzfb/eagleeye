#include "eagleeye/algorithm/saliency.h"
namespace eagleeye{
Saliency::Saliency(){
	//this parameter guide how to quantize the img
	m_quantized_num = 10;
	m_ratio = 0.95f;
	m_sigma_dist = 0.4f;
	m_saliency_type = RC;
}

Saliency::~Saliency(){

}

Matrix<float> Saliency::run(Matrix<unsigned char> img){
	//transform to matrix with float type
	Matrix<float> valid_img_mat = img.transform(NormalizeOperation<unsigned char,float>(0,255,0.0f,1.0f));
	m_img = cv::Mat(valid_img_mat.rows(),valid_img_mat.cols(),CV_32F);
	memcpy(m_img.data,valid_img_mat.dataptr(),sizeof(float) * valid_img_mat.rows() * valid_img_mat.cols());

	switch(m_saliency_type)
	{
	case HC:
		{
            std::cout<<"in HC"<<std::endl;
			computeHC();
			break;
		}
	case RC:
		{
			computeRC();
			break;
		}
	default:
		{
			return Matrix<float>();
		}
	}

	//save to output image signal
	Matrix<float> saliency_img(m_img_saliency.rows,m_img_saliency.cols,(float*)m_img_saliency.data,false);
    return saliency_img;
}

void Saliency::quantize()
{
	//quantize m_img
	float clr_tmp = m_quantized_num - 0.0001f;

	//m_img_index store the hist index of every pixel
	m_img_color_index = cv::Mat::zeros(m_img.size(),CV_32S);
	int rows = m_img.rows;
	int cols = m_img.cols;

	if (m_img.isContinuous() && m_img_color_index.isContinuous())
	{
		cols *= rows;
		rows = 1;
	}

	//build color pallet
	//(color,num)
	std::vector<int> pallet(m_quantized_num,0);

	for (int i = 0; i < rows; ++i)
	{
		const float* img_data = m_img.ptr<float>(i);
		int* idx = m_img_color_index.ptr<int>(i);
		for (int j = 0; j < cols; ++j)
		{
			//find the color index, and store it in m_img_index
			idx[j] = (int)(img_data[j] * clr_tmp);
			pallet[idx[j]]++;
		}
	}

	//Fine significant colors
	int max_num = 0;
	std::vector<std::pair<int,int>> num;//(num, color) pairs in num

	int pallet_num = pallet.size();
	for (int i = 0; i < pallet_num; ++i)
	{
		num.push_back(std::pair<int,int>(pallet[i],i));//(color, num) pairs in pallet
	}

	//sort data in the descending order
	std::sort(num.begin(),num.end(),std::greater<std::pair<int,int>>());

	//clip %5 data from the tail
	max_num=num.size();
	int max_drop_num = cvRound(rows * cols * (1.0f - m_ratio));
	for (int crnt = num[max_num - 1].first; crnt < max_drop_num && max_num > 1; --max_num)
	{
		crnt += num[max_num - 2].first;
	}

	max_num = EAGLEEYE_MIN(max_num,256);//to avoid very rarely case
	if (max_num < 10)
	{
		max_num = EAGLEEYE_MIN(pallet.size(),100);
	}

	//we believe that the front max_num elements of "num" store
	//the primary data
	//We will reassign value for pallet
	// pallet->(color,index)
	pallet.clear();
	pallet.resize(m_quantized_num,0);
	for (int i = 0; i < max_num; ++i)
	{
		//reassign pallet value
		//now pallet[] corresponds to one label
		pallet[num[i].second] = i;
	}

	//find the nearest value for the back max_num elements of "num"
	//"color" stores the color index in the quantization space
	std::vector<int> color(num.size());
	for (unsigned int i = 0; i < num.size(); ++i)
	{
		color[i] = num[i].second;
	}

	for (int i = max_num; i < int(num.size()); ++i)
	{
		int sim_idx = 0, sim_val = INT_MAX;
		for (int j = 0; j < max_num; ++j)
		{
			int d_ij = abs(color[i] - color[j]);
			if (d_ij < sim_val)
			{
				sim_val = d_ij;
				sim_idx = j;
			}
		}

		pallet[num[i].second] = pallet[num[sim_idx].second];
	}

	m_hist_color = cv::Mat::zeros(1,max_num,CV_32F);
	m_hist_color_num = cv::Mat::zeros(m_hist_color.size(),CV_32S);

	float* hist_color_data = m_hist_color.ptr<float>(0);
	int* hist_color_num_data = m_hist_color_num.ptr<int>(0);

	for (int i = 0; i < rows; ++i)
	{
		float* img_data = m_img.ptr<float>(i);
		int* idx = m_img_color_index.ptr<int>(i);
		for (int j = 0; j < cols; ++j)
		{
			idx[j] = pallet[idx[j]];
			hist_color_data[idx[j]] += img_data[j];
			hist_color_num_data[idx[j]]++;
		}
	}

	for (int i = 0; i < max_num; ++i)
	{
		if (hist_color_num_data[i] != 0)
		{
			hist_color_data[i] /= hist_color_num_data[i];
		}
	}
}

void Saliency::computeHC()
{
	quantize();
	cv::Mat weight;
	normalize(m_hist_color_num,weight,1,0,cv::NORM_L1,CV_32F);
	//////////////////////////////////////////////////////////////////////////

	int bin_n = m_hist_color.cols;
	cv::Mat hist_color_saliency = cv::Mat::zeros(1,bin_n,CV_32F);

	float* color_scal_data = hist_color_saliency.ptr<float>(0);
	std::vector<std::vector<CostIdx>> similarity(bin_n);
	float* color_data = m_hist_color.ptr<float>(0);
	float* weight_data = weight.ptr<float>(0);

	for (int i = 0; i < bin_n; ++i)
	{
		std::vector<CostIdx> &cur_sim=similarity[i];
		for (int j = 0; j < bin_n; ++j)
		{
			if (i == j)
			{
				continue;
			}
			//the distance between color
			//Actually, this distance is measured between "color index"
			float dij = abs(color_data[i] - color_data[j]);
			cur_sim.push_back(std::make_pair(dij,j));
			color_scal_data[i] += weight_data[j] * dij;//see formula (3)
		}

		//sort cur_sim
		sort(similarity[i].begin(),similarity[i].end());
	}

	//smoothing for saliency, we need to consider weight
	//see formula (4)
	smoothSaliency(m_hist_color,hist_color_saliency,4.0f,similarity);

	//////////////////////////////////////////////////////////////////////////
	//now we could compute image saliency by 
	// using m_hist_color_saliency
	int rows = m_img.rows;
	int cols = m_img.cols;
	m_img_saliency = cv::Mat(rows,cols,CV_32F);
	float* hist_color_saliency_data = hist_color_saliency.ptr<float>(0);
	for (int i = 0; i < rows; ++i)
	{
		float* saliency_row_data = m_img_saliency.ptr<float>(i);
		int* index_data = m_img_color_index.ptr<int>(i);
		for (int j = 0; j < cols; ++j)
		{
			saliency_row_data[j] = hist_color_saliency_data[index_data[j]];
		}
	}

	normalize(m_img_saliency,m_img_saliency,0,1,cv::NORM_MINMAX);
}

void Saliency::smoothSaliency(const cv::Mat& bin_color,cv::Mat& scal,float delta,const std::vector<std::vector<CostIdx>> &similarity)
{
	int bin_n = bin_color.cols;
	const float* bin_color_data = bin_color.ptr<float>(0);

	cv::Mat tmp_scal;
	scal.copyTo(tmp_scal);
	float *scal_data = tmp_scal.ptr<float>(0);
	float *n_scal_data = scal.ptr<float>(0);

	//distance based smooth
	int n = EAGLEEYE_MAX(cvRound(bin_n / delta),2);

	std::vector<float> dist(n,0);
	std::vector<float> val(n,0);

	for (int i = 0; i < bin_n; ++i)
	{
		const std::vector<CostIdx> &cur_sim = similarity[i];
		float total_dist = 0;
		val[0] = scal_data[i];

		for (int j = 1; j < n; ++j)
		{
			int ith_idx = cur_sim[j].second;
			dist[j] = cur_sim[j].first;
			val[j] = scal_data[ith_idx];
			total_dist += dist[j];
		}

		float val_crnt = 0;
		for (int j = 0; j < n; ++j)
		{
			val_crnt += val[j] * (total_dist - dist[j]);
		}

		n_scal_data[i] = val_crnt / ((n - 1) * total_dist);
	}
}

void Saliency::computeRC()
{
	//////////////////////////////////////////////////////////////////////////
	//quantization 
	quantize();

	Matrix<unsigned char> used_img(m_img.rows,m_img.cols,(unsigned char)0);
	for (int i = 0; i < m_img.rows; ++i)
	{
		unsigned char* used_img_data = used_img.row(i);
		float* img_data = m_img.ptr<float>(i);
		for (int j = 0; j < m_img.cols; ++j)
		{
			used_img_data[j] = (unsigned char)(img_data[j] * 255);
		}
	}

    SRM srm = SRM();
    Matrix<int> label_map = srm.run(used_img);
	int reg_num = srm.getLabelNum();	
	int rows = label_map.rows();
	int cols = label_map.cols();

	cv::Mat reg_label(rows,cols,CV_32S);
	memcpy(reg_label.data,label_map.dataptr(),sizeof(unsigned int) * rows * cols);

	std::vector<_Region> regs(reg_num);
	buildRegion(reg_label,regs,m_img_color_index,m_hist_color.cols);

	cv::Mat region_saliency;
	regionContrast(regs,m_hist_color,region_saliency,m_sigma_dist);

	//////////////////////////////////////////////////////////////////////////
	m_img_saliency = cv::Mat::zeros(rows,cols,CV_32F);

	for (int i = 0; i < rows; ++i)
	{
		const unsigned int* reg_label_data = reg_label.ptr<unsigned int>(i);
		float* img_saliency_data = m_img_saliency.ptr<float>(i);
		for (int j = 0; j < cols; ++j)
		{
			img_saliency_data[j] = region_saliency.at<float>(reg_label_data[j]);
		}
	}

	normalize(m_img_saliency,m_img_saliency,0,1,cv::NORM_MINMAX);
}

void Saliency::regionContrast(const std::vector<_Region> &regs,
												const cv::Mat& hist_color,
												cv::Mat& saliency,
												float sigma_dist)
{
	//compute color distance table
	cv::Mat c_dist_table = cv::Mat::zeros(hist_color.cols,hist_color.cols,CV_32F);
	const float* hist_color_data = hist_color.ptr<float>(0);

	//compute the distance between different color
	for (int i = 0; i < c_dist_table.rows; ++i)
	{
		for (int j = i + 1; j < c_dist_table.cols; ++j)
		{
			c_dist_table.at<float>(i,j) = c_dist_table.at<float>(j,i) = abs(hist_color_data[i] - hist_color_data[j]);
		}
	}


	int reg_num = regs.size();
	cv::Mat r_dist_table = cv::Mat::zeros(reg_num,reg_num,CV_32F);

	//the saliency of different regions
	saliency = cv::Mat::zeros(1,reg_num,CV_32F);
	float* saliency_data = saliency.ptr<float>(0);
	for (int i = 0; i < reg_num; ++i)
	{
		for (int j = 0; j < reg_num; ++j)
		{
			if (i < j)
			{
				float dd = 0.0f;
				const std::vector<CostIdx> &c1 = regs[i].fre_idx;
				const std::vector<CostIdx> &c2 = regs[j].fre_idx;

				int c1_num = int(c1.size());
				int c2_num = int(c2.size());

				for (int m = 0; m < c1_num; ++m)
				{
					for (int n = 0; n < c2_num; ++n)
					{
						//see formula (6)
						//c1[m].first is the probability of different color among n color in
						// this region
						// c2[n].first
						dd += c1[m].first * c2[n].first * c_dist_table.at<float>(c1[m].second,c2[n].second);
					}
				}

				float x_dis = float(regs[i].centroid.x - regs[j].centroid.x);
				float y_dis = float(regs[i].centroid.y - regs[j].centroid.y);
				float temp_dist = sqrt(x_dis * x_dis + y_dis * y_dis);

				r_dist_table.at<float>(j,i) = r_dist_table.at<float>(i,j) = 
					dd * exp(-temp_dist / sigma_dist);
			}

			saliency_data[i] += regs[j].pix_num * r_dist_table.at<float>(i,j);
		}

        
	}
}

void Saliency::buildRegion(const cv::Mat& reg_label,
										std::vector<_Region>& regs,
										const cv::Mat& img_color_index, 
										int color_num)
{
	int rows = reg_label.rows;
	int cols = reg_label.cols;
	int reg_num = regs.size();

	cv::Mat reg_color_fre = cv::Mat::zeros(reg_num,color_num,CV_32S);
	for (int i = 0; i < rows; ++i)
	{
		//the region index of pixel
		const unsigned int* reg_idx = reg_label.ptr<unsigned int>(i);

		//the color index of pixel
		const int* color_idx = img_color_index.ptr<int>(i);

		for (int j = 0; j < cols; ++j)
		{
			_Region &reg = regs[reg_idx[j]];
			reg.pix_num++;
			reg.centroid.x += j;
			reg.centroid.y += i;

			reg_color_fre.at<int>(reg_idx[j],color_idx[j])++;
		}
	}

	for (int i = 0; i < reg_num; ++i)
	{
		_Region& reg = regs[i];

		//normalize centroid coordinate
		reg.centroid.x /= (reg.pix_num * cols);
		reg.centroid.y /= (reg.pix_num * rows);
		int* reg_color_fre_data = reg_color_fre.ptr<int>(i);
		for (int j = 0; j < color_num; ++j)
		{
			//normalize to 0~1
			float fre = (float)reg_color_fre_data[j] / (float)reg.pix_num;
			//see formula (6)
			if (fre > 0)
			{
				reg.fre_idx.push_back(std::make_pair(fre,j));
			}
		}
	}
}
}