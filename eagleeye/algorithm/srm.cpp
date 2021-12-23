#include "eagleeye/algorithm/srm.h"
#include "eagleeye/basic/Variable.h"
#include <math.h>
namespace eagleeye
{
SRM::SRM(){
	m_pf_average = NULL;
	m_pn_count = NULL;
	m_pn_neighbor_bucket = NULL;
	m_pn_next_neighbor = NULL;
	m_pn_region_index = NULL;
	m_fg = 256.0f;
	m_fq = 50.0f;
	m_n_region_count = 0;

	m_n_width = 0;
	m_n_height = 0;
	m_n_pixel_number = 0;
	m_n_depth = 0;
	m_n_img_pixel_number = 0;
}

SRM::~SRM(){
	if(m_pf_average)
	{
		delete[] m_pf_average;
		m_pf_average = NULL;
	}
	if(m_pn_count)
	{
		delete[] m_pn_count;
		m_pn_count = NULL;
	}
	if(m_pn_neighbor_bucket)
	{
		delete[] m_pn_neighbor_bucket;
		m_pn_neighbor_bucket = NULL;
	}
	if(m_pn_next_neighbor)
	{
		delete[] m_pn_next_neighbor;
		m_pn_next_neighbor = NULL;
	}

	if(m_pn_region_index)
	{
		delete[] m_pn_region_index;
		m_pn_region_index = NULL;
	}

}

Matrix<int> SRM::getLabelMap()
{
	return m_label_map;
}

Matrix<float> SRM::getGrayMap()
{
	return m_gray_map;
}

int SRM::getLabelNum()
{
	return m_n_region_count;
}

Matrix<int> SRM::run(Matrix<unsigned char> input_img){
	// unsigned char max_value;
	// unsigned char min_value;
	// getMaxMin(input_img,max_value,min_value);
	// m_input_img = input_img.transform(NormalizeOperation<unsigned char,unsigned char>(min_value,max_value,0,255));
    this->m_input_img = input_img;
	m_label_map = Matrix<int>(m_input_img.rows(),m_input_img.cols());
	m_gray_map = Matrix<float>(m_input_img.rows(),m_input_img.cols());
	
    // std::cout<<"initilize "<<std::endl;
	//initialize some basic parameters
	initialize(m_input_img.cols(),m_input_img.rows());

    // std::cout<<"after initialize"<<std::endl;
	unsigned char* puch_srcptr = (unsigned char*)m_input_img.dataptr();

	int* label_map_data = (int*)m_label_map.dataptr();
	float* gray_map_data = (float*)m_gray_map.dataptr();
    // std::cout<<puch_srcptr<<std::endl;
    // std::cout<<label_map_data<<std::endl;
    // std::cout<<gray_map_data<<std::endl;

	unsigned int region_count;
	
	//start segmentation
    // std::cout<<"start process2d"<<std::endl;
	process2D(puch_srcptr,label_map_data,region_count,gray_map_data);

	// //get output image signal
	// ImageSignal<int>* label_image_signal = dynamic_cast<ImageSignal<int>*>(m_output_signals[0]);
	// ImageSignal<float>* gray_image_signal = dynamic_cast<ImageSignal<float>*>(m_output_signals[1]);
	// ImageSignal<int>* number_signal = dynamic_cast<ImageSignal<int>*>(m_output_signals[2]);

	// if ((!label_image_signal) || (!gray_image_signal) || (!number_signal)){
	// 	EAGLEEYE_ERROR("sorry,output image is not correct\n please be careful...\n");
	// 	return;
	// }

	// label_image_signal->img = m_label_map;
	// gray_image_signal->img = m_gray_map;
	// Matrix<int> label_num(1,1);
	// label_num(0) = getLabelNum();
	// number_signal->img = label_num;

    return this->m_label_map;
}

int SRM::initialize(int n_width, int n_height,int n_depth /* = 1 */)
{
	n_depth = 1;
	if(n_width < 1 || n_height < 1 || n_depth < 1)
		return -1;
	recycleGarbage();

	m_n_width = n_width;
	m_n_height = n_height;
	m_n_pixel_number = m_n_width * m_n_height;
	m_n_depth = n_depth;
	m_n_img_pixel_number=m_n_pixel_number * m_n_depth;

	m_pf_average = new float[m_n_img_pixel_number];
	m_pn_count = new int[m_n_img_pixel_number];
	m_pn_region_index = new int[m_n_img_pixel_number];

	m_pn_next_neighbor = new int[2 * m_n_img_pixel_number];

	// bucket sort
	m_pn_neighbor_bucket = new int[256];

	return 0;
}

int SRM::process2D(unsigned char* puch_src,int* pui_label,unsigned int& ui_region_count,float* pf_average /* = NULL */)
{
    // std::cout<<"step 1"<<std::endl;
	if (puch_src == NULL || pui_label == NULL){
        // std::cout<<"step 1 why here"<<std::endl;
		return -1;
    }

    // std::cout<<"step 2"<<std::endl;
	m_puch_srcptr = puch_src;
	srm2D(pui_label,pf_average);
	ui_region_count = (unsigned int)m_n_region_count;

	return 0;
}

void SRM::srm2D(int* pui_label,float* pf_average /* = NULL */)
{
	m_f_delta = 1.f/(6*m_n_width*m_n_height);
	/*
	* This would be the non-relaxed formula:
	* factor = g*g/2/Q*(float)Math.log(2/delta);
	* The paper claims that this is more prone to oversegmenting.
	*/
	m_f_factor = m_fg*m_fg/2/m_fq;
	m_f_log_delta = 2.f*(float)log(float(6*m_n_width*m_n_height));
	
    // std::cout<<"init region2d"<<std::endl;
	initRegions2D();
    // std::cout<<"init neighbors2d"<<std::endl;
	initNeighbors2D();
    // std::cout<<"merge nerghbors2d"<<std::endl;
	mergeAllNeighbors2D();
	

	if(pf_average) 
	{
		for (int i=0; i<m_n_pixel_number; i++)
			pf_average[i] = m_pf_average[getRegionIndex(i)];
	}
    // std::cout<<"consolidateregion"<<std::endl;
	m_n_region_count = consolidateRegions();
	

	if(m_n_region_count > 1<<16)
	{
        // std::cout<<"why here"<<std::endl;
		return;
	}
	for(int i=0; i<m_n_pixel_number; i++)
		pui_label[i] = (int) m_pn_region_index[i];
}

void SRM::initRegions2D()
{
	for(int i = 0; i < m_n_pixel_number; i++)
	{
		m_pf_average[i] = (float)m_puch_srcptr[i];
		m_pn_count[i] = 1;
		m_pn_region_index[i] = i;
	}
}

void SRM::initNeighbors2D()
{
	int i, j;
	for(i = 0; i < 256; i++)
		m_pn_neighbor_bucket[i] = -1;

	for(j = m_n_height - 1; j >= 0; j--)
	{
		for(i = m_n_width - 1; i >= 0; i--)
		{
			int n_index = i + m_n_width * j;
			int n_neighbor_index = 2 * n_index;
			// vertical
			if(j < m_n_height - 1)
				addNeighborPair(n_neighbor_index + 1,n_index,n_index + m_n_width);
			// horizontal
			if(i < m_n_width - 1)
				addNeighborPair(n_neighbor_index,n_index,n_index + 1);
		}
	}
}

void SRM::addNeighborPair(int n_neighbor_index, int i1, int i2)
{
	int n_difference = abs((int)m_puch_srcptr[i1] - m_puch_srcptr[i2]);
	m_pn_next_neighbor[n_neighbor_index] = m_pn_neighbor_bucket[n_difference];
	m_pn_neighbor_bucket[n_difference] = n_neighbor_index;
}

void SRM::mergeAllNeighbors2D()
{
	for(int i = 0; i < 256; i++) // 256Ϊm_pn_neighbor_bucket�ĳ���
	{
		int neighbor_index = m_pn_neighbor_bucket[i];
		while(neighbor_index >= 0) 
		{
			int i1 = neighbor_index / 2;
			int i2 = i1+(0 == (neighbor_index&1)? 1 : m_n_width);

			i1 = getRegionIndex(i1);
			i2 = getRegionIndex(i2);

			if(predicate(i1, i2))
				mergeRegions(i1, i2);

			neighbor_index = m_pn_next_neighbor[neighbor_index];
		}
	}
}

// recursively find out the region index for this pixel
int SRM::getRegionIndex(int i)
{
	i = m_pn_region_index[i];
	while(i < 0)
		i = m_pn_region_index[-1-i];
	return i;
}

// should regions i1 and i2 be merged?
bool SRM::predicate(int i1, int i2)
{
	float difference = m_pf_average[i1] - m_pf_average[i2];
	// This would be the non-relaxed predicate mentioned in the paper.
	// return difference*difference < factor*(1f/count[i1]+1f/count[i2]);
	float log1 = (float)log(float(1 + m_pn_count[i1])) * (m_fg < m_pn_count[i1] ? m_fg:m_pn_count[i1]);
	float log2 = (float)log(float(1 + m_pn_count[i2])) * (m_fg < m_pn_count[i2] ? m_fg:m_pn_count[i2]);
	return difference * difference < 0.1f * m_f_factor * ((log1 + m_f_log_delta) / m_pn_count[i1] + 
		((log2 + m_f_log_delta) / m_pn_count[i2]));

}

void SRM::mergeRegions(int i1, int i2)
{
	if (i1 == i2)
		return;
	int merged_count = m_pn_count[i1] + m_pn_count[i2];
	float merged_average = (m_pf_average[i1] * m_pn_count[i1] + m_pf_average[i2] * m_pn_count[i2]) / merged_count;

	// merge larger index into smaller index
	if(i1>i2) 
	{
		m_pf_average[i2] = merged_average;
		m_pn_count[i2] = merged_count;
		m_pn_region_index[i1] = -1 - i2;
	}
	else 
	{
		m_pf_average[i1] = merged_average;
		m_pn_count[i1] = merged_count;
		m_pn_region_index[i2] = -1 - i1;
	}
}

/*
* By construction, a negative regionIndex will always point
* to a smaller regionIndex.
*
* So we can get away by iterating from small to large and
* replacing the positive ones with running numbers, and the
* negative ones by the ones they are pointing to (that are
* now guaranteed to contain a non-negative index).
*/
int SRM::consolidateRegions()
{
	int count = 0;
	for(int i=0; i< m_n_img_pixel_number; i++)
	{
		if(m_pn_region_index[i] < 0)
			m_pn_region_index[i] = m_pn_region_index[-1 - m_pn_region_index[i]];
		else
			m_pn_region_index[i] = count++;
	}
	return count;
}

void SRM::recycleGarbage()
{
	if(m_pf_average)
	{
		delete[] m_pf_average;
		m_pf_average = NULL;
	}
	if(m_pn_count)
	{
		delete[] m_pn_count;
		m_pn_count = NULL;
	}
	if(m_pn_neighbor_bucket)
	{
		delete[] m_pn_neighbor_bucket;
		m_pn_neighbor_bucket = NULL;
	}
	if(m_pn_next_neighbor)
	{
		delete[] m_pn_next_neighbor;
		m_pn_next_neighbor = NULL;
	}

	if(m_pn_region_index)
	{
		delete[] m_pn_region_index;
		m_pn_region_index = NULL;
	}
}

Matrix<float> SRM::getSegmentationCenters()
{
	Matrix<float> segmentation_centers(m_n_region_count,2,0.0f);
	std::vector<int> count_record(m_n_region_count,0);

	int rows = m_label_map.rows();
	int cols = m_label_map.cols();
	
	//gain the pixel number of every segmentation
	for (int i = 0; i < rows; ++i)
	{
		int* row_data = m_label_map.row(i);
		for (int j = 0; j < cols; ++j)
		{
			count_record[row_data[j]] = count_record[row_data[j]] + 1;
		}
	}

	//gain center seeds
	std::vector<int> center_seeds(m_n_region_count);
	for (int i = 0; i < m_n_region_count; ++i)
	{
		Variable<int> var = Variable<int>::uniform(0,count_record[i]);
		center_seeds[i] = var.var();
	}

	//find center position
	count_record.clear();
	count_record.resize(m_n_region_count,0);
	for (int i = 0; i < rows; ++i)
	{
		int* row_data = m_label_map.row(i);
		for (int j = 0; j < cols; ++j)
		{
			if (count_record[row_data[j]] == center_seeds[row_data[j]])
			{
				segmentation_centers(row_data[j],0) = float(j);
				segmentation_centers(row_data[j],1) = float(i);
			}
			count_record[row_data[j]] = count_record[row_data[j]] + 1;
		}
	}
	
	return segmentation_centers;
}

Matrix<int> SRM::getPixelNumOfSegmentation()
{
	Matrix<int> count_record(m_n_region_count,1);

	int rows = m_label_map.rows();
	int cols = m_label_map.cols();

	//gain the pixel number of every segmentation
	for (int i = 0; i < rows; ++i)
	{
		int* row_data = m_label_map.row(i);
		for (int j = 0; j < cols; ++j)
		{
			count_record[row_data[j]] = count_record[row_data[j]] + 1;
		}
	}

	return count_record;
}

} // namespace eagleeye

