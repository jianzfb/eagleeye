#include "eagleeye/engine/nano/op/arcfacealign_op.h"

namespace eagleeye{
namespace dataflow{
cv::Mat meanAxis0(const cv::Mat &src)
{
    int num = src.rows;
    int dim = src.cols;

    // x1 y1
    // x2 y2

    cv::Mat output(1,dim,CV_32F);
    for(int i = 0 ; i <  dim; i ++)
    {
        float sum = 0 ;
        for(int j = 0 ; j < num ; j++)
        {
            sum+=src.at<float>(j,i);
        }
        output.at<float>(0,i) = sum/num;
    }

    return output;
}

cv::Mat elementwiseMinus(const cv::Mat &A,const cv::Mat &B)
{
    cv::Mat output(A.rows,A.cols,A.type());

    assert(B.cols == A.cols);
    if(B.cols == A.cols)
    {
        for(int i = 0 ; i <  A.rows; i ++)
        {
            for(int j = 0 ; j < B.cols; j++)
            {
                output.at<float>(i,j) = A.at<float>(i,j) - B.at<float>(0,j);
            }
        }
    }
    return output;
}


cv::Mat varAxis0(const cv::Mat &src)
{
    cv::Mat temp_ = elementwiseMinus(src,meanAxis0(src));
    cv::multiply(temp_ ,temp_ ,temp_ );
    return meanAxis0(temp_);
}



int MatrixRank(cv::Mat M)
{
    cv::Mat w, u, vt;
    cv::SVD::compute(M, w, u, vt);
    cv::Mat1b nonZeroSingularValues = w > 0.0001;
    int rank = countNonZero(nonZeroSingularValues);
    return rank;
}

//    References
//    ----------
//    .. [1] "Least-squares estimation of transformation parameters between two
//    point patterns", Shinji Umeyama, PAMI 1991, DOI: 10.1109/34.88573
//
//    """
//
//    Anthor:Jack Yu
cv::Mat similarTransform(cv::Mat src,cv::Mat dst) {
    int num = src.rows;
    int dim = src.cols;
    cv::Mat src_mean = meanAxis0(src);
    cv::Mat dst_mean = meanAxis0(dst);
    cv::Mat src_demean = elementwiseMinus(src, src_mean);
    cv::Mat dst_demean = elementwiseMinus(dst, dst_mean);
    cv::Mat A = (dst_demean.t() * src_demean) / static_cast<float>(num);
    cv::Mat d(dim, 1, CV_32F);
    d.setTo(1.0f);
    if (cv::determinant(A) < 0) {
        d.at<float>(dim - 1, 0) = -1;

    }
    cv::Mat T = cv::Mat::eye(dim + 1, dim + 1, CV_32F);
        cv::Mat U, S, V;
    cv::SVD::compute(A, S,U, V);

    // the SVD function in opencv differ from scipy .


    int rank = MatrixRank(A);
    if (rank == 0) {
        assert(rank == 0);

    } else if (rank == dim - 1) {
        if (cv::determinant(U) * cv::determinant(V) > 0) {
            T.rowRange(0, dim).colRange(0, dim) = U * V;
        } else {
//            s = d[dim - 1]
//            d[dim - 1] = -1
//            T[:dim, :dim] = np.dot(U, np.dot(np.diag(d), V))
//            d[dim - 1] = s
            int s = d.at<float>(dim - 1, 0) = -1;
            d.at<float>(dim - 1, 0) = -1;

            T.rowRange(0, dim).colRange(0, dim) = U * V;
            cv::Mat diag_ = cv::Mat::diag(d);
            cv::Mat twp = diag_*V; //np.dot(np.diag(d), V.T)
            cv::Mat B = cv::Mat::zeros(3, 3, CV_8UC1);
            cv::Mat C = B.diag(0);
            T.rowRange(0, dim).colRange(0, dim) = U* twp;
            d.at<float>(dim - 1, 0) = s;
        }
    }
    else{
        cv::Mat diag_ = cv::Mat::diag(d);
        cv::Mat twp = diag_*V.t(); //np.dot(np.diag(d), V.T)
        cv::Mat res = U* twp; // U
        T.rowRange(0, dim).colRange(0, dim) = -U.t()* twp;
    }
    cv::Mat var_ = varAxis0(src_demean);
    float val = cv::sum(var_).val[0];
    cv::Mat res;
    cv::multiply(d,S,res);
    float scale =  1.0/val*cv::sum(res).val[0];
    T.rowRange(0, dim).colRange(0, dim) = - T.rowRange(0, dim).colRange(0, dim).t();
    cv::Mat  temp1 = T.rowRange(0, dim).colRange(0, dim); // T[:dim, :dim]
    cv::Mat  temp2 = src_mean.t(); //src_mean.T
    cv::Mat  temp3 = temp1*temp2; // np.dot(T[:dim, :dim], src_mean.T)
    cv::Mat temp4 = scale*temp3;
    T.rowRange(0, dim).colRange(dim, dim+1)=  -(temp4 - dst_mean.t()) ;
    T.rowRange(0, dim).colRange(0, dim) *= scale;
    return T;
}



ArcFaceAlignOp::ArcFaceAlignOp(){
    m_src_handler = 0;
    m_tgt_handler = 0;

    m_src_ptr = NULL;
    m_tgt_ptr = NULL;

    this->m_margin = 10;
    this->m_margin_ratio = 0.15;

    m_dst_points = cv::Mat(5,2,CV_32F);
    m_dst_points.at<float>(0,0) = 38.2946;
    m_dst_points.at<float>(0,1) = 51.6963;
    m_dst_points.at<float>(1,0) = 73.5318;
    m_dst_points.at<float>(1,1) = 51.5014;
    m_dst_points.at<float>(2,0) = 56.0252;
    m_dst_points.at<float>(2,1) = 71.7366;
    m_dst_points.at<float>(3,0) = 41.5493;
    m_dst_points.at<float>(3,1) = 92.3655;
    m_dst_points.at<float>(4,0) = 70.7299;
    m_dst_points.at<float>(4,1) = 92.2041;
}
ArcFaceAlignOp::ArcFaceAlignOp(int target_h, int target_w, int margin){
    this->m_target_h = target_h;
    this->m_target_w = target_w;
    this->m_margin = margin;

    m_src_handler = 0;
    m_tgt_handler = 0;

    m_src_ptr = NULL;
    m_tgt_ptr = NULL;
}

ArcFaceAlignOp::~ArcFaceAlignOp(){
#ifdef EAGLEEYE_RKCHIP    
    if (m_src_handler){
        releasebuffer_handle(m_src_handler);
    }
    if (m_tgt_handler){
        releasebuffer_handle(m_tgt_handler);
    }
#endif
}

int ArcFaceAlignOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("target_h") != params.end()){
        this->m_target_h = params["target_h"][0];
    }
    if(params.find("target_w") != params.end()){
        this->m_target_w = params["target_w"][0];
    }

    if(params.find("margin") != params.end()){
        this->m_margin = params["margin"][0];
    }
    if(params.find("margin_ratio") != params.end()){
        this->m_margin_ratio = params["margin_ratio"][0];
    }
    return 0;
}

int ArcFaceAlignOp::runOnCpu(const std::vector<Tensor>& input){
    Tensor image = input[0];
    Tensor bbox = input[1];
    Tensor kps = input[2];
    Dim image_dim = image.dims();
    if(image_dim.size() == 4){
        EAGLEEYE_LOGE("ArcFaceAlignOp dont support batch image");
        return -1;
    }

    int image_h = image_dim[0];
    int image_w = image_dim[1];
    int image_c = image_dim[2];
    unsigned char* image_ptr = image.cpu<unsigned char>();

    if(bbox.dims()[0] == 0){
        EAGLEEYE_LOGD("No face.");
        this->m_outputs[0] = 
            Tensor(std::vector<int64_t>{0, this->m_target_w, image_c}, image.type(), image.format(),CPU_BUFFER);
        return 0;
    }

    float* bbox_ptr = bbox.cpu<float>();
    if(bbox_ptr[0] < 0 || bbox_ptr[1] < 0 || bbox_ptr[2] < 0 || bbox_ptr[3] < 0){
        // 对于无效人脸，返回全0人脸图
        this->m_outputs[0] = 
            Tensor(std::vector<int64_t>{this->m_target_h, this->m_target_w, image_c}, image.type(), image.format(),CPU_BUFFER);        
        return 0;
    }

    float face_cx = (bbox_ptr[0]+bbox_ptr[2])/2.0f;
    float face_cy = (bbox_ptr[1]+bbox_ptr[3])/2.0f;
    float face_w = bbox_ptr[2]-bbox_ptr[0];
    float face_h = bbox_ptr[3]-bbox_ptr[1];

    float face_half_size = std::max(face_w, face_h) / 2;
    float margin = m_margin;
    float face_half_width = face_half_size + margin;
    float face_half_height = face_half_size + margin;
    if(this->m_margin_ratio > 0){
        face_half_width = face_half_size + m_margin_ratio * face_w;
        face_half_height = face_half_size + m_margin_ratio * face_h;
    }

    // 保证4宽度对齐（对于一些硬件加速使用）
    int x0 = int(face_cx - face_half_width + 0.5f);
    x0 = std::max(x0, 0) / 4 * 4;
    int y0 = int(face_cy - face_half_height + 0.5f);
    y0 = std::max(y0, 0) / 4 * 4;
    int x1 = int(face_cx + face_half_width + 0.5f);
    x1 = (std::min(x1+3, image_w)) / 4 * 4;
    int y1 = int(face_cy + face_half_height + 0.5f);
    y1 = (std::min(y1+3, image_h)) / 4 * 4;

    face_w = x1 - x0;
    face_h = y1 - y0;
    if(face_w == 0 || face_h == 0){
        // 对于无效人脸，返回全0人脸图
        this->m_outputs[0] = 
            Tensor(std::vector<int64_t>{this->m_target_h, this->m_target_w, image_c}, image.type(), image.format(),CPU_BUFFER);        
        return 0;
    }

    if(this->m_outputs[0].numel() != this->m_target_h*this->m_target_w*image_c){
        Dim out_dim(std::vector<int64_t>{this->m_target_h, this->m_target_w, image_c});
        this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);
    }
    unsigned char* output_ptr = this->m_outputs[0].cpu<unsigned char>();
    
    cv::Mat ori_image(image_h, image_w, CV_8UC3, image_ptr);
    cv::Mat tgt_image(m_target_h, m_target_w, CV_8UC3, output_ptr);
    float ratio = float(m_target_w)/112.0f;
    // float diff_x = 0.0f;

    float* kps_ptr = kps.cpu<float>();
    cv::Mat lmk_points(5, 2, CV_32F, kps_ptr);
    cv::Mat dst_points = m_dst_points * ratio;

    cv::Mat rot_mat = similarTransform(lmk_points, dst_points);
    cv::warpPerspective(ori_image, tgt_image, rot_mat, tgt_image.size());
    return 0;
}

int ArcFaceAlignOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}