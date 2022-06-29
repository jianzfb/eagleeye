#include "eagleeye/engine/nano/op/factory.h"
#include "eagleeye/engine/nano/op/cast_op.h"
#include "eagleeye/engine/nano/op/concat_op.h"
#include "eagleeye/engine/nano/op/interpolate_op.h"
#include "eagleeye/engine/nano/op/pad2d_op.h"
#include "eagleeye/engine/nano/op/reduce_op.h"
#include "eagleeye/engine/nano/op/repeat_op.h"
#include "eagleeye/engine/nano/op/reshape_op.h"
#include "eagleeye/engine/nano/op/transpose_op.h"
#include "eagleeye/engine/nano/op/clip_op.h"
#include "eagleeye/engine/nano/op/preprocess_op.h"
#include "eagleeye/engine/nano/op/placeholder_op.h"
#include "eagleeye/engine/nano/op/const_op.h"
#include "eagleeye/engine/nano/op/split_op.h"
#include "eagleeye/engine/nano/op/shape_op.h"
#include "eagleeye/engine/nano/op/image_resize_op.h"
#include "eagleeye/engine/nano/op/slice_op.h"

namespace eagleeye{
namespace dataflow{
Node* build_node_op(Graph* g, std::string op_name, std::string op_cls, neb::CJsonObject op_config){
    if(op_cls == "PlaceholderOp"){
        std::string data_format;
        op_config.Get("data_format", data_format);

        neb::CJsonObject shape_obj;
        op_config.Get("shape", shape_obj);
        
        std::vector<int> shape(4);
        shape_obj.Get(0, shape[0]);
        shape_obj.Get(1, shape[1]);
        shape_obj.Get(2, shape[2]);
        shape_obj.Get(3, shape[3]);

        std::map<std::string, std::vector<float>> params;
        params["shape"] = std::vector<float>({(float)(shape[0]), (float)(shape[1]), (float)(shape[2]), (float)(shape[3])});
        params["memory_type"] = std::vector<float>({2});
        if(data_format == "NHWC"){
            params["data_format"] = std::vector<float>({(float)(DataFormat::NHWC)});
        }
        else if(data_format == "NCHW"){
            params["data_format"] = std::vector<float>({(float)(DataFormat::NCHW)});
        }
        else{
            params["data_format"] = std::vector<float>({(float)(DataFormat::NC)});
        }
        std::string data_type;
        op_config.Get("data_type", data_type);
        if(data_type == "UCHAR"){
            params["data_type"] = std::vector<float>({(float)(EAGLEEYE_UCHAR)});
        }
        else if(data_type == "CHAR"){
            params["data_type"] = std::vector<float>({(float)(EAGLEEYE_CHAR)});
        }
        else if(data_type == "FLOAT"){
            params["data_type"] = std::vector<float>({(float)(EAGLEEYE_FLOAT)});
        }
        else if(data_type == "INT"){
            params["data_type"] = std::vector<float>({(float)(EAGLEEYE_INT)});
        }

        Node* node = g->add(op_name, PlaceholderOp(), EagleeyeRuntime(EAGLEEYE_CPU));
        node->init(params);
        return node;
    }
    else if(op_cls == "CastOp"){
        std::string data_type_str;
        EagleeyeType data_type = EAGLEEYE_UNDEFINED;
        op_config.Get("data_type", data_type_str);
        if(data_type_str == "UCHAR"){
            data_type = EAGLEEYE_UCHAR;
        }
        else if(data_type_str == "CHAR"){
            data_type = EAGLEEYE_CHAR;
        }
        else if(data_type_str == "FLOAT"){
            data_type = EAGLEEYE_FLOAT;
        }
        float scale  = 1.0f;
        op_config.Get("scale", scale);

        Node* node = g->add(op_name, CastOp(data_type, scale), EagleeyeRuntime(EAGLEEYE_CPU));
        return node;
    }
    else if(op_cls == "ConcatOp"){
        int axis = -1;
        op_config.Get("axis", axis);
        Node* node = g->add(op_name, ConcatOp(axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node;
    }
    else if(op_cls == "ClipOp"){
        float min_v = 0.0f;
        float max_v = 0.0f;
        op_config.Get("min_v", min_v);
        op_config.Get("max_v", max_v);
        Node* node = g->add(op_name, ClipOp(min_v, max_v), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "InterpolateOp"){
        neb::CJsonObject shape_obj;
        op_config.Get("out_size", shape_obj);
        
        std::vector<int64_t> out_size(2);
        int out_h = 1;
        int out_w = 1;
        shape_obj.Get(0, out_h);
        shape_obj.Get(1, out_w);
        out_size[0] = out_h;
        out_size[1] = out_w;

        float scale = 0.0f;
        op_config.Get("scale", scale);
        bool align_corner = true;
        op_config.Get("align_corner", align_corner);

        std::string interpolate_type_str;
        op_config.Get("interpolate_type", interpolate_type_str);
        InterpolateOpType interpolate_type = INTERPOLATE_BILINER;
        if(interpolate_type_str == "BILINER"){
            interpolate_type = INTERPOLATE_BILINER;
        }
        else{
            interpolate_type = INTERPOLATE_NEAREST;
        }

        Node* node = g->add(op_name, InterpolateOp(out_size, scale, align_corner, interpolate_type), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "InterpolateWithShapeOp"){
        bool align_corner = true;
        op_config.Get("align_corner", align_corner);

        std::string interpolate_type_str;
        op_config.Get("interpolate_type", interpolate_type_str);
        InterpolateOpType interpolate_type = INTERPOLATE_BILINER;
        if(interpolate_type_str == "BILINER"){
            interpolate_type = INTERPOLATE_BILINER;
        }
        else{
            interpolate_type = INTERPOLATE_NEAREST;
        }

        Node* node = g->add(op_name, InterpolateWithShapeOp(align_corner, interpolate_type), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "Pad2dOp"){
        std::string pad_type_str;
        op_config.Get("pad_type", pad_type_str);
        Pad2dOpType pad_type;
        if(pad_type_str == "CONSTANT"){
            pad_type = Pad2dOpType::PAD2D_CONSTANT;
        }
        else if(pad_type_str == "EDGE"){
            pad_type = Pad2dOpType::PAD2D_EDGE;
        }
        else{
            pad_type = Pad2dOpType::PAD2D_REFLECT;
        }

        std::vector<int64_t> pad_c;
        std::vector<int64_t> pad_h(2);
        std::vector<int64_t> pad_w(2);

        // 忽略pad_c
        neb::CJsonObject pad_h_obj;
        op_config.Get("pad_h", pad_h_obj);
        pad_h_obj.Get(0, pad_h[0]);
        pad_h_obj.Get(1, pad_h[1]);

        neb::CJsonObject pad_w_obj;
        op_config.Get("pad_w", pad_w_obj);
        pad_w_obj.Get(0, pad_w[0]);
        pad_w_obj.Get(1, pad_w[1]);

        float pad_value=0.0f;
        op_config.Get("pad_value", pad_value);

        Node* node = g->add(op_name, 
                            Pad2dOp(pad_type, pad_c, pad_h, pad_w,pad_value), 
                            EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "ConstOp"){
        float val;
        op_config.Get("val", val);

        neb::CJsonObject shape_obj;
        op_config.Get("shape", shape_obj);
        std::vector<int64_t> shape(shape_obj.GetArraySize());
        for(int i=0; i<shape_obj.GetArraySize(); ++i){
            shape_obj.Get(i, shape[i]);
        }

        Node* node = g->add(op_name,
                            ConstOp<float>(val, shape, "CPU"));
        return node;
    }
    else if(op_cls == "ReduceMinOp"){
        bool keep_axis = true;
        op_config.Get("keep_axis", keep_axis);

        neb::CJsonObject axis_obj;
        op_config.Get("axis", axis_obj);
        std::vector<int64_t> axis(axis_obj.GetArraySize());
        for(int i=0; i<axis_obj.GetArraySize(); ++i){
            axis_obj.Get(i, axis[i]);
        }

        Node* node = g->add(op_name, ReduceMinOp(axis, keep_axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "ReduceMaxOp"){
        bool keep_axis = true;
        op_config.Get("keep_axis", keep_axis);

        neb::CJsonObject axis_obj;
        op_config.Get("axis", axis_obj);
        std::vector<int64_t> axis(axis_obj.GetArraySize());
        for(int i=0; i<axis_obj.GetArraySize(); ++i){
            axis_obj.Get(i, axis[i]);
        }

        Node* node = g->add(op_name, ReduceMaxOp(axis, keep_axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "ReduceMeanOp"){
        bool keep_axis = true;
        op_config.Get("keep_axis", keep_axis);

        neb::CJsonObject axis_obj;
        op_config.Get("axis", axis_obj);
        std::vector<int64_t> axis(axis_obj.GetArraySize());
        for(int i=0; i<axis_obj.GetArraySize(); ++i){
            axis_obj.Get(i, axis[i]);
        }

        Node* node = g->add(op_name, ReduceMeanOp(axis, keep_axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "ReduceSumOp"){
        bool keep_axis = true;
        op_config.Get("keep_axis", keep_axis);

        neb::CJsonObject axis_obj;
        op_config.Get("axis", axis_obj);
        std::vector<int64_t> axis(axis_obj.GetArraySize());
        for(int i=0; i<axis_obj.GetArraySize(); ++i){
            axis_obj.Get(i, axis[i]);
        }

        Node* node = g->add(op_name, ReduceSumOp(axis, keep_axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "ReduceProdOp"){
        bool keep_axis = true;
        op_config.Get("keep_axis", keep_axis);

        neb::CJsonObject axis_obj;
        op_config.Get("axis", axis_obj);
        std::vector<int64_t> axis(axis_obj.GetArraySize());
        for(int i=0; i<axis_obj.GetArraySize(); ++i){
            axis_obj.Get(i, axis[i]);
        }

        Node* node = g->add(op_name, ReduceProdOp(axis, keep_axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "RepeatOp"){
        int repeat_times = 1;
        int axis = 0;
        op_config.Get("repeat_times", repeat_times);
        op_config.Get("axis", axis);

        Node* node = g->add(op_name, RepeatOp(repeat_times, axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "ReshapeOp"){
        neb::CJsonObject shape_obj;
        op_config.Get("shape", shape_obj);
        std::vector<int64_t> shape(shape_obj.GetArraySize());
        for(int i=0; i<shape_obj.GetArraySize(); ++i){
            shape_obj.Get(i, shape[i]);
        }
        bool in_place = false;

        Node* node = g->add(op_name, ReshapeOp(shape, in_place), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "TransposeOp"){
        neb::CJsonObject axis_obj;
        op_config.Get("axis", axis_obj);
        std::vector<int64_t> axis(axis_obj.GetArraySize());
        for(int i=0; i<axis_obj.GetArraySize(); ++i){
            axis_obj.Get(i, axis[i]);
        }   

        Node* node = g->add(op_name, TransposeOp(axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "PreprocessOp"){
        neb::CJsonObject mean_obj;
        std::vector<float> mean_v(3);
        op_config.Get("mean", mean_obj);
        mean_obj.Get(0, mean_v[0]);
        mean_obj.Get(1, mean_v[1]);
        mean_obj.Get(2, mean_v[2]);

        neb::CJsonObject scale_obj;
        std::vector<float> scale_v(3);
        op_config.Get("scale", scale_obj);
        scale_obj.Get(0, scale_v[0]);
        scale_obj.Get(1, scale_v[1]);
        scale_obj.Get(2, scale_v[2]);

        bool reverse_channel = false;
        op_config.Get("reverse_channel", reverse_channel);
        
        Node* node = g->add(op_name, PreprocessOp(mean_v, scale_v, reverse_channel), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "Split2DOp"){
        int axis = 0;
        op_config.Get("axis", axis);

        Node* node = g->add(op_name, Split2DOp(axis), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "ShapeOp"){
        int start = -1;
        op_config.Get("start", start);
        int stop = -1;
        op_config.Get("stop", stop);
        Node* node = g->add(op_name, ShapeOp(start, stop), EagleeyeRuntime(EAGLEEYE_CPU));
        return node;  
    }
    else if(op_cls == "ImageResizeOp"){
        neb::CJsonObject shape_obj;
        op_config.Get("out_size", shape_obj);
        
        std::vector<int64_t> out_size(2);
        int out_h = 1;
        int out_w = 1;
        shape_obj.Get(0, out_h);
        shape_obj.Get(1, out_w);
        out_size[0] = out_h;
        out_size[1] = out_w;

        float scale = 0.0f;
        op_config.Get("scale", scale);

        // std::string interpolate_type_str;
        // op_config.Get("interpolate_type", interpolate_type_str);
        // InterpolateOpType interpolate_type = INTERPOLATE_BILINER;
        // if(interpolate_type_str == "BILINER"){
        //     interpolate_type = INTERPOLATE_BILINER;
        // }
        // else{
        //     interpolate_type = INTERPOLATE_NEAREST;
        // }

        Node* node = g->add(op_name, ImageResizeOp(out_size, scale, INTERPOLATE_BILINER), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "ImageResizeWithShapeOp"){
        // std::string interpolate_type_str;
        // op_config.Get("interpolate_type", interpolate_type_str);
        // InterpolateOpType interpolate_type = INTERPOLATE_BILINER;
        // if(interpolate_type_str == "BILINER"){
        //     interpolate_type = INTERPOLATE_BILINER;
        // }
        // else{
        //     interpolate_type = INTERPOLATE_NEAREST;
        // }

        Node* node = g->add(op_name, ImageResizeWithShapeOp(INTERPOLATE_BILINER), EagleeyeRuntime(EAGLEEYE_CPU));
        return node; 
    }
    else if(op_cls == "SliceOp"){
        neb::CJsonObject axis_obj;
        op_config.Get("axis", axis_obj);

        neb::CJsonObject starts_obj;
        op_config.Get("starts", starts_obj);

        neb::CJsonObject ends_obj;
        op_config.Get("ends", ends_obj);

        std::vector<int> axis;
        std::vector<int> starts;
        std::vector<int> ends;
        for(int i=0; i<axis_obj.GetArraySize(); ++i){
            int v = -1;
            axis_obj.Get(i, v);
            if(v == -1){
                continue;
            }

            int s,e;
            starts_obj.Get(i,s);
            ends_obj.Get(i,e);
            axis.push_back(v);
            starts.push_back(s);
            ends.push_back(e);
        }

        Node* node = g->add(op_name, SliceOp(axis,starts,ends), EagleeyeRuntime(EAGLEEYE_CPU));
        return node;  
    }
    return NULL;
}    

} // namespace dataflow
    
} // namespace eagleeye
