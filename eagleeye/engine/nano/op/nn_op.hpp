#if defined(RKNN_NN_ENGINE)
#include "eagleeye/engine/nano/op/rknn_op.hpp"
namespace eagleeye{
    namespace dataflow{
        template<std::size_t IN, std::size_t OUT>
        using NNOp = RknnOp<IN, OUT>;
    }
}

#elif defined(TENSORRT_NN_ENGINE)
#include "eagleeye/engine/nano/op/tensorrt_op.hpp"
namespace eagleeye{
    namespace dataflow{
        template<std::size_t IN, std::size_t OUT>
        using NNOp = TensorrtOp<IN, OUT>;
    }
}
#elif defined(SNPE_NN_ENGINE)
#include "eagleeye/engine/nano/op/snpe_op.hpp"
namespace eagleeye{
    namespace dataflow{
        template<std::size_t IN, std::size_t OUT>
        using NNOp = SnpeOp<IN, OUT>;
    }
}
#elif defined(TNN_NN_ENGINE)
#include "eagleeye/engine/nano/op/tnn_op.hpp"
namespace eagleeye{
    namespace dataflow{
        template<std::size_t IN, std::size_t OUT>
        using NNOp = TnnOp<IN, OUT>;
    }
}
#endif

