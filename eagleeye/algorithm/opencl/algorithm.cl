__kernel void homography_eva(__global float* source_points, __global float* h, __global float* target_points,__global float* result,const unsigned int h_num, const unsigned int num){
    int h_index = get_global_id(0);
    int p_index = get_global_id(1);

    float* h_offset = h + h_index * 9;
    if(h_index < h_num && p_index < num){
        float x = source_points[p_index*3];
        float y = source_points[p_index*3+1];
        float z = source_points[p_index*3+2];
        float projected_x = h_offset[0] * x + h_offset[1] * y + h_offset[2] * z;
        float projected_y = h_offset[3] * x + h_offset[4] * y + h_offset[5] * z;
        float projected_z = h_offset[6] * x + h_offset[7] * y + h_offset[8] * z;

        result[h_index*num+p_index] = (fabs(projected_x/projected_z - target_points[p_index*3]) + fabs(projected_y/projected_z - target_points[p_index*3+1]))/2.0f;
    }
}

__kernel void binaryFeatures(__global float* I, __global unsigned int* F, int sx, int sy, int N, int N_space, __constant float* const_sampPatch){
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sx || y >= sy)
        return;

    int offset = x + y * sx;
    int offsetF = x * N_space + y * N_space * sx;
    float i1, i2;
    unsigned int ff;
    int c = 1, n = -1, ox1, ox2, oy1, oy2;
    for (unsigned int s=0; s<N; s++){
        if (s%32 == 0){
            if (n>=0){
                F[offsetF + n] = ff;
            }

            ff = 0; c = 0;
            n++;
        }

        ox1 = max(-x, min( sx-x-1, (int)const_sampPatch[4*s]));
        oy1 = max(-y, min( sy-y-1, (int)const_sampPatch[4*s+1]));
        ox2 = max(-x, min( sx-x-1, (int)const_sampPatch[4*s+2]));
        oy2 = max(-y, min( sy-y-1, (int)const_sampPatch[4*s+3]));

        i1 = I[offset + ox1 + oy1*sx];
        i2 = I[offset + ox2 + oy2*sx];
        ff += ((i1 > i2) << c);
        c++;
    }
    F[offsetF + n] = ff;
}

inline float featDist(__global unsigned int* F1, __global unsigned int* F2, 
                        float dmax,
                        int offset, 
                        int nx, int ny, 
                        int sx, int N, int N_space)
{
    float ddmax = N*dmax;
    unsigned short dist = 0;
    
    for (unsigned int s=0; s<N_space; s++)
    {
        unsigned int bd = (F1[offset+s] ^ F2[offset+s + nx*N_space + ny*sx*N_space]);
        dist += popcount(bd);
    }
    
    return ((float)dist)/N;
}

__kernel void init(__global float* I1, __global float* I2,
                     __global unsigned int* F1, __global unsigned int* F2,
                     __global float *D, __global short *NX, __global short *NY,
                     int sx, int sy,
                     int first, int N, int N_space)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sx || y >= sy)
        return;

    int offset = x + y * sx;
    int offsetF = x * N_space + y * N_space * sx;
    
    if (first == 1)
    {
        D[offset] = featDist(F1,F2,1000,offsetF,0,0,sx,N,N_space);
        NX[offset] = 0;
        NY[offset] = 0;
    }
    else
    {
        short nx0, ny0, nx, ny;
        
        nx0 = NX[offset];
        ny0 = NY[offset];
        nx = min(max((int)(NX[offset+nx0+ny0*sx]), -x), sx-x-1),
        ny = min(max((int)(NY[offset+nx0+ny0*sx]), -y), sy-y-1);
        
        D[offset] = featDist(F1,F2,1000,offsetF,nx,ny,sx,N,N_space);
        
        NX[offset] = nx;
        NY[offset] = ny;
    }
}

inline bool insert(__global float* I1, __global float* I2, __global unsigned int* F1, __global unsigned int* F2,
                   __global float *D, __global short *NX, __global short *NY,
                   int offset, int offsetF, int nx, int ny, int sx, int sy,
                   int N, int N_space)
{
    if (nx == NX[offset] && ny == NY[offset]){
        return false;
    }
    
    float dst = featDist(F1,F2,D[offset],offsetF,nx,ny,sx,N,N_space);
        
    if (dst < 0){
        return false;
    }
    else if (dst < D[offset]){
        D[offset]  = dst;
        NX[offset] = nx;
        NY[offset] = ny;
    }
    
    return true;
}


__kernel void propagate(__global float* I1, __global float* I2,
                          __global unsigned int* F1, __global unsigned int* F2,
                          __global float *D, __global short *NX, __global short *NY,
                          int sx, int sy,
                          int pd, int jfMax, int doubleProp,
                          int N, int N_space)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sx || y >= sy)
        return;

    int offset = x + y * sx;
    int offsetF = x * N_space + y * N_space * sx;
    
    short nx, ny;
    int doff, dstart = doubleProp==1 ? -1 : 1;
    int doff_x, doff_y;
    for (int p=jfMax; p>0; p/=2)
    {
        for (int d=dstart; d<=1; d+=2)
        {
            for (int xy=0; xy<=1; xy++)
            {
                doff_x = pd*p*d*xy;
                doff_y = pd*p*d*(1-xy);
                if(x+doff_x < 0 || x+doff_x >= sx){
                    continue;
                }
                if(y+doff_y < 0 || y+doff_y >= sy){
                    continue;
                }

                doff = doff_x + sx * doff_y;
                nx = min(max((int)NX[offset + doff], -x), sx-x-1);
                ny = min(max((int)NY[offset + doff], -y), sy-y-1);
                
                insert(I1, I2, F1, F2, D, NX, NY, offset, offsetF, nx, ny, sx, sy, N, N_space);
            }
        }
    }
}

inline unsigned int rand(unsigned int* seed){
    int const a = 16807;
    int const m = 2147483647;

    *seed = ((long)((*seed) * a))%m;
    return(*seed);
}

__kernel void randomSearch(__global float* I1, __global float* I2,
                             __global unsigned int* F1, __global unsigned int* F2,
                             __global float *D, __global short *NX, __global short *NY, __global int *seed_memory,
                             int sx, int sy, float dist,
                             int N, int N_space)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sx || y >= sy)
        return;

    int offset = x + y * sx;
    int offsetF = x * N_space + y * N_space * sx;
    
    unsigned int seed = seed_memory[offset];
    unsigned int nx_rand = rand(&seed) % 1000000;
    float nx_rand_f = (float)(nx_rand)/1000000.0f;
    nx_rand_f = 2.0f*nx_rand_f - 1.0f;
    short nx = min(max((int)round(dist*nx_rand_f), -x), sx-x-1);

    unsigned int ny_rand = rand(&seed) % 1000000;
    float ny_rand_f = (float)(ny_rand)/1000000.0f;
    ny_rand_f = 2.0f*ny_rand_f - 1.0f;
    short ny = min(max((int)round(dist*ny_rand_f), -y), sy-y-1);
    seed_memory[offset] = seed;

    insert(I1, I2, F1, F2, D, NX, NY, offset, offsetF, nx, ny, sx, sy, N, N_space);
}

__kernel void medianFilterS1D(__global short *Li, __global short *Lo, int dir, int fsize, int sx, int sy)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sx || y >= sy)
        return;

    int i = 0;
    int ks = fsize/2;
    short v[47];
    
    if (dir)
    {
        for (int xx = x - ks; xx <= x + ks; xx++)
            v[i++] = Li[xx+y*sx];
    }
    else
    {
        for (int yy = y - ks; yy <= y + ks; yy++)
            v[i++] = Li[x+yy*sx];
    }
    
    short tmp;
    for (unsigned int i=0; i<=ks; i++){
        for (unsigned int j=i+1; j<fsize; j++){
            if (v[i] > v[j])
            {
                tmp = v[i];
                v[i] = v[j];
                v[j] = tmp;
            }
        }
    }

    Lo[x + y*sx] = v[ks];
}


__kernel void filter1D(__global float *Li, __global float *Lo,
                        int ks, 
                        int dir, 
                        int sx, int sy,
                        __constant float* const_kernel)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sx || y >= sy)
        return;        
    int offset = x + y * sx;

    float s = 0;
    int ks2 = ks/2;
    
    if (dir)
    {
        for (int i=0; i<ks; i++)
            s += const_kernel[i]*Li[min(sx-1,max(0,x+i-ks2)) + y*sx];
    }
    else
    {
        for (int i=0; i<ks; i++)
            s += const_kernel[i]*Li[x + min(sy-1,max(0,y+i-ks2))*sx];
    }
    
    Lo[offset] = s;
}


__kernel void crossTrilateralFilter(__global float *Ui, __global float *Vi,
                                __global float *Uo, __global float *Vo,
                                __global float *Uim, __global float *Vim,
                                __global float *I,
                                float sigmaEPE, float sigmaI, float sigmaS,
                                int sx, int sy)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sx || y >= sy)
        return;     

    int offset = x + y*sx;
    
    int s = ceil(2.5*sqrt(sigmaS)), offset1;
    float um0 = Uim[offset], u1, du,
          vm0 = Vim[offset], v1, dv,
          I0 = I[offset]/255, I1;
          
    float w, dEPE, dI;

    float uf = 0, vf = 0, W = 0;
    for (int i=-s; i<s; i++)
        for (int j=-s; j<s; j++)
        {
            offset1 = min(sx-1,max(0,x+i)) + min(sy-1,max(0,y+j))*sx;
            u1 = Ui[offset1];
            v1 = Vi[offset1];
            I1 = I[offset1]/255;
            
            du = u1-um0; dv = v1-vm0;
            
            dEPE = (du*du+dv*dv);
            dI = (I1-I0);
            w = exp(-(dEPE)/(2*sigmaEPE)) * exp(-(dI*dI)/(2*sigmaI)) * exp(-(i*i + j*j)/(2*sigmaS));
            
            uf += w*u1;
            vf += w*v1;
            W += w;
        }
    
    Uo[offset] = uf/max(1e-5,W);
    Vo[offset] = vf/max(1e-5,W);
}

__kernel void convert(__global short *Li, __global float *Lo, int sx, int sy)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sx || y >= sy)
        return;     
    
    Lo[x + y*sx] = Li[x + y*sx];
}

__kernel void resizeAndCombineToFlowXY(__global float* flow_x, __global float* flow_y, __global float* flow_xy, int sxi, int syi, int sxo, int syo){
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= sxo || y >= syo)
        return;     

    if(sxo != sxi || syo != syi){
        float x_f = ((float)(x)/ (float)(sxo)) * (sxi);
        float y_f = ((float)(y)/ (float)(syo)) * (syi);

        int y_i = (int)(y_f);
        int x_i = (int)(x_f);
        float u = y_f - y_i;
        float v = x_f - x_i;

        int p1_y = min(y_i, syi - 1);
        int p1_x = min(x_i, sxi - 1);
        float flow_x_p1 = flow_x[p1_x + p1_y * sxi];
        float flow_y_p1 = flow_y[p1_x + p1_y * sxi];

        int p2_y = p1_y;
        int p2_x = min(x_i + 1, sxi - 1);
        float flow_x_p2 = flow_x[p2_x + p2_y * sxi];
        float flow_y_p2 = flow_y[p2_x + p2_y * sxi];

        int p3_y = min(y_i + 1, syi - 1);
        int p3_x = x_i;
        float flow_x_p3 = flow_x[p3_x + p3_y * sxi];
        float flow_y_p3 = flow_y[p3_x + p3_y * sxi];

        int p4_y = p3_y;
        int p4_x = p2_x;
        float flow_x_p4 = flow_x[p4_x + p4_y * sxi];
        float flow_y_p4 = flow_y[p4_x + p4_y * sxi];

        float p1_w = (1 - u) * (1 - v);
        float p2_w = (1 - u) * v;
        float p3_w = u * (1 - v);
        float p4_w = u * v;

        flow_xy[(x + y*sxo)*2] = p1_w*flow_x_p1+p2_w*flow_x_p2+p3_w*flow_x_p3+p4_w*flow_x_p4;
        flow_xy[(x + y*sxo)*2+1] = p1_w*flow_y_p1+p2_w*flow_y_p2+p3_w*flow_y_p3+p4_w*flow_y_p4;
    }
    else{
        flow_xy[(x + y*sxo)*2] = flow_x[x + y*sxi];
        flow_xy[(x + y*sxo)*2+1] = flow_y[x + y*sxi];
    }
}