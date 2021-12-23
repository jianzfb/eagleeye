#include "eagleeye/algorithm/filter.h"
namespace eagleeye
{
Filter::Filter(){
    this->m_sigmaI = 5.0f;
    this->m_sigmaS = 5.0f;
}   

Filter::~Filter(){

} 

void Filter::fastBF(Matrix<float> data, Matrix<float>& filted_data){
    int rows = data.rows();
    int cols = data.cols();
    filted_data = Matrix<float>(rows, cols);
    
    this->fastLBF(data, data, filted_data,this->m_sigmaI,this->m_sigmaS);
}

void Filter::crossFastBF(Matrix<float> data, Matrix<float> reference, Matrix<float>& filted_data){
    int rows = data.rows();
    int cols = data.cols();
    filted_data = Matrix<float>(rows, cols);
    
    this->fastLBF(data, reference, filted_data, this->m_sigmaI,this->m_sigmaS);
}

void Filter::setSigmaI(float sigmai){
    this->m_sigmaI = sigmai;
}
void Filter::setSigmaS(float sigmas){
    this->m_sigmaS = sigmas;
}

inline float trilinear_interpolation(const float* mat,
                                     const size_t height,
                                     const size_t width,
                                     const size_t depth,
                                     const size_t offset_y,
                                     const size_t offset_x,
                                     const size_t offset_z,
                                     const float y,
                                     const float x,
                                     const float z)
{
    const size_t y_index = eagleeye_clip(size_t(y), 0, height-1);
    const size_t yy_index = eagleeye_clip(y_index+1, 0, height-1);    

    const size_t x_index  = eagleeye_clip(size_t(x), 0, width-1);
    const size_t xx_index = eagleeye_clip(x_index+1, 0, width-1);

    const size_t z_index  = eagleeye_clip(size_t(z), 0, depth-1);
    const size_t zz_index = eagleeye_clip(z_index+1, 0, depth-1);

    const float y_alpha = y - y_index;
    const float x_alpha = x - x_index;
    const float z_alpha = z - z_index;

    float y_x_z_v = mat[y_index*offset_y + x_index*offset_x + z_index*offset_z];
    float y_xx_z_v = mat[y_index*offset_y + xx_index*offset_x + z_index*offset_z];
    float yy_x_z_v = mat[yy_index*offset_y + x_index*offset_x + z_index*offset_z];
    float yy_xx_z_v = mat[yy_index*offset_y + xx_index*offset_x + z_index*offset_z];
    float y_x_zz_v = mat[y_index*offset_y + x_index*offset_x + zz_index*offset_z];
    float y_xx_zz_v = mat[y_index*offset_y + xx_index*offset_x + zz_index*offset_z];
    float yy_x_zz_v = mat[yy_index*offset_y + x_index*offset_x + zz_index*offset_z];
    float yy_xx_zz_v = mat[yy_index*offset_y + xx_index*offset_x + zz_index*offset_z];

    return
        (1.0-y_alpha) * (1.0-x_alpha) * (1.0-z_alpha) * y_x_z_v +
        (1.0-y_alpha) * x_alpha       * (1.0-z_alpha) * y_xx_z_v +
        y_alpha       * (1.0-x_alpha) * (1.0-z_alpha) * yy_x_z_v +
        y_alpha       * x_alpha       * (1.0-z_alpha) * yy_xx_z_v +
        (1.0-y_alpha) * (1.0-x_alpha) * z_alpha       * y_x_zz_v +
        (1.0-y_alpha) * x_alpha       * z_alpha       * y_xx_zz_v +
        y_alpha       * (1.0-x_alpha) * z_alpha       * yy_x_zz_v +
        y_alpha       * x_alpha       * z_alpha       * yy_xx_zz_v;

}

void Filter::fastLBF(Matrix<float> src, 
                         Matrix<float> reference,
                         Matrix<float>& dst,
                         float sigma_color, 
                         float sigma_space)
{
   const size_t height = src.rows(), width = src.cols();
    const size_t padding_xy = 2, padding_z = 2;
    float reference_min = 10000000.0f;
    float reference_max = -100000000.0f;
    for(int i=0; i<height; ++i){
        float* reference_ptr = reference.row(i);
        for(int j=0; j<width; ++j){
            if(reference_min >= reference_ptr[j]){
                reference_min = reference_ptr[j];
            }
            if(reference_max <= reference_ptr[j]){
                reference_max = reference_ptr[j];
            }
        }
    }

    const size_t small_height = static_cast<size_t>((height-1)/sigma_space) + 1 + 2 * padding_xy;
    const size_t small_width  = static_cast<size_t>((width-1)/sigma_space) + 1 + 2 * padding_xy;
    const size_t small_depth  = static_cast<size_t>((reference_max-reference_min)/sigma_color) + 1 + 2 * padding_z;

    float* grid_data = new float[small_height*small_width*small_depth];
    memset(grid_data, 0, sizeof(float)*small_height*small_width*small_depth);

    float* grid_vote = new float[small_height*small_width*small_depth];
    memset(grid_vote, 0, sizeof(float)*small_height*small_width*small_depth);
    
    int offset_y = small_width*small_depth;
    int offset_x = small_depth;
    int offset_z = 1;

    // down sample
    for ( int y = 0; y < height; ++y ) {
        for ( int x = 0; x < width; ++x) {
            const size_t small_x = size_t(float(x)/sigma_space + 0.5f) + padding_xy;
            const size_t small_y = size_t(float(y)/sigma_space + 0.5f) + padding_xy;
            const float z = reference.at(y,x) - reference_min;
            const size_t small_z = size_t(z/sigma_color + 0.5f) + padding_z;

            grid_data[small_y*offset_y+small_x*offset_x+small_z] += src.at(y,x);
            grid_vote[small_y*offset_y+small_x*offset_x+small_z] += 1.0f;
        }
    }

    // convolution
    int offset[3];
    offset[0] = offset_x; offset[1] = offset_y; offset[2] = offset_z;
    float* buffer1 = new float[small_height*small_width*small_depth];
    memset(buffer1, 0, sizeof(float)*small_height*small_width*small_depth);
    float* buffer2 = new float[small_height*small_width*small_depth];
    memset(buffer2, 0, sizeof(float)*small_height*small_width*small_depth);

    for ( int dim = 0; dim < 3; ++dim ) { // dim = 3 stands for x, y, and depth
        const int off = offset[dim];
        for ( int ittr = 0; ittr < 2; ++ittr ) {
            std::swap(grid_data, buffer1);
            std::swap(grid_vote, buffer2);

            for ( int y = 1; y < small_height-1; ++y ) {
                for ( int x = 1; x < small_width-1; ++x ) {
                    int ptr_offset = y*offset_y + x*offset_x + 1;
                    float* d_ptr = grid_data + ptr_offset;
                    float* v_ptr = grid_vote + ptr_offset;

                    float* b1_ptr = buffer1 + ptr_offset;
                    float* b2_ptr = buffer2 + ptr_offset;

                    for ( int z = 1; z < small_depth-1; ++z, ++d_ptr, ++b1_ptr, ++v_ptr, ++b2_ptr ) {
                        float b1_prev = *(b1_ptr-off), b1_curr = *(b1_ptr), b1_next = *(b1_ptr+off);
                        float b2_prev = *(b2_ptr-off), b2_curr = *(b2_ptr), b2_next = *(b2_ptr+off);
                        *d_ptr = (b1_prev + b1_next + 2.0 * b1_curr) / 4.0;
                        *v_ptr = (b2_prev + b2_next + 2.0 * b2_curr) / 4.0;
                    } // z
                } // x
            } // y

        } // ittr
    } // dim

    // upsample
    int total = small_height*small_width*small_depth;
    for(int i=0; i<total; ++i){
        grid_data[i] /= (grid_vote[i] > 0.000000000001f ?  float(grid_vote[i]) : 1.0f);
    }

    for ( int y = 0; y < height; ++y ) {
        for ( int x = 0; x < width; ++x ) {
            const float px = float(x) / sigma_space + padding_xy;
            const float py = float(y) / sigma_space + padding_xy;
            const float z = reference.at(y,x) - reference_min;
            const float pz = float(z) / sigma_color + padding_z;

            dst.at(y,x) = trilinear_interpolation(grid_data,
                                                  small_height,
                                                  small_width,
                                                  small_depth,
                                                  offset_y,
                                                  offset_x,
                                                  offset_z,
                                                  py, 
                                                  px, 
                                                  pz);

        }
    }

    delete[] grid_data;
    delete[] grid_vote;
    delete[] buffer1;
    delete[] buffer2;
}

} // namespace eagleeye
