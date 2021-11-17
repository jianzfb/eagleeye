#include "eagleeye/algorithm/llsp.h"

namespace eagleeye{
Matrix<float> linearLSP(Matrix<float> a, Matrix<float> b, float tol, Matrix<float>& rsd){
    int m = a.rows();
    int n = a.cols();
    assert(b.rows() == m);
    assert(b.cols() == 1);

    float* aa_ptr = (float*)malloc(m*n*sizeof(float));
    int count = 0;
    for(int j=0; j<n; ++j){
        for(int i=0; i<m; ++i){
            aa_ptr[count] = a.at(i,j);
            count += 1;
        }
    }

    Matrix<float> bb = b;
    if(!bb.isfull()){
        bb = bb.clone();
    }    
    Matrix<float> x(n,1);

    float* b_ptr = bb.dataptr();
    float* x_ptr = x.dataptr();
    rsd = Matrix<float>(m, 1);
    float* rsd_ptr = rsd.dataptr();
    int* jpvt = (int*)malloc(n * sizeof(int));
    float* qraux = (float*)malloc(n* sizeof(float));
    int itask = 1;
    int kr=0;
    dqrls(aa_ptr,m,m,n,tol,&kr,b_ptr,x_ptr,rsd_ptr,jpvt,qraux,itask);

    free(jpvt);
    free(qraux);
    free(aa_ptr);
    return x;
}
 
}