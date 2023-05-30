namespace eagleeye {
namespace math {
inline int GetElementwiseIndex(const int64_t *x_dims_array,
                               const int max_dim,
                               const int *index_array) {
  int index_ = 0;
  for (int i = 0; i < max_dim; i++) {
    if (x_dims_array[i] > 1) {
      index_ = index_ * x_dims_array[i] + index_array[i];
    }
  }
  return index_;
}

inline void UpdateElementwiseIndexArray(const int64_t *out_dims_array,
                                        const int max_dim,
                                        int *index_array) {
  for (int i = max_dim - 1; i >= 0; --i) {
    ++index_array[i];
    if (index_array[i] >= out_dims_array[i]) {
      index_array[i] -= out_dims_array[i];
    } else {
      break;
    }
  }
}

template <typename Functor, typename T, typename OutType = T>
void CommonForwardBroadcast(const T *x_data,
                            const T *y_data,
                            OutType *out_data,
                            int64_t *x_dims_array,
                            int64_t *y_dims_array,
                            int64_t *out_dims_array,
                            int max_dim,
                            Functor func) {
  std::vector<int> index_array(max_dim, 0);

  const int out_size = std::accumulate(
      out_dims_array, out_dims_array + max_dim, 1, std::multiplies<int64_t>());
  int x_index, y_index;
  for (int out_index = 0; out_index < out_size; ++out_index) {
    x_index = GetElementwiseIndex(x_dims_array, max_dim, index_array.data());
    y_index = GetElementwiseIndex(y_dims_array, max_dim, index_array.data());
    out_data[out_index] = func(x_data[x_index], y_data[y_index]);

    UpdateElementwiseIndexArray(out_dims_array, max_dim, index_array.data());
  }
}

inline Dim trim_trailing_singular_dims(const Dim &dims) {
  // Remove trailing dimensions of size 1 for y
  auto actual_dims_size = dims.size();
  for (; actual_dims_size != 0; --actual_dims_size) {
    if (dims[actual_dims_size - 1] != 1) break;
  }
  if (actual_dims_size == dims.size()) return dims;
  std::vector<int64_t> trim_dims;
  trim_dims.resize(actual_dims_size);
  for (int i = 0; i < actual_dims_size; ++i) {
    trim_dims[i] = dims[i];
  }
  if (trim_dims.size() == 0) {
    return Dim();
  }
  Dim actual_dims = Dim(trim_dims);
  return actual_dims;
}

template <typename Functor, typename T, typename OutType = T>
void CommonElementwiseBroadcastForward(const T *x,
                                       const T *y,
                                       OutType *z,
                                       const Dim &x_dims,
                                       const Dim &y_dims,
                                       const Dim &out_dims,
                                       Functor func) {
  int max_dim = std::max(x_dims.size(), y_dims.size());

  std::vector<int64_t> x_dims_array = x_dims.Vectorize();
  std::vector<int64_t> y_dims_array = y_dims.Vectorize();
  std::vector<int64_t> out_dims_array = out_dims.Vectorize();

  CommonForwardBroadcast<Functor, T, OutType>(x,
                                              y,
                                              z,
                                              x_dims_array.data(),
                                              y_dims_array.data(),
                                              out_dims_array.data(),
                                              max_dim,
                                              func);
}

inline void get_mid_dims(const Dim &x_dims,
                         const Dim &y_dims,
                         const int axis,
                         int *pre,
                         int *n,
                         int *post,
                         int *is_run_common_broadcast) {
  *pre = 1;
  *n = 1;
  *post = 1;
  for (int i = 0; i < axis; ++i) {
    (*pre) *= x_dims[i];
  }

  for (int i = 0; i < y_dims.size(); ++i) {
    // do broadcast
    if (x_dims[i + axis] != y_dims[i]) {
      *is_run_common_broadcast = 1;
    }
    (*n) *= y_dims[i];
  }

  for (int i = axis + y_dims.size(); i < x_dims.size(); ++i) {
    (*post) *= x_dims[i];
  }
}

template <typename PType, typename CompareFunctor>
void CompareComputeRun(const Tensor& X, const Tensor& Y, Tensor& Out, int64_t broadcast_axis) {
  using DType = typename CompareFunctor::TYPE;
  const size_t x_size = X.numel();
  const size_t y_size = Y.numel();
  auto x_dims = X.dims();
  auto y_dims = Y.dims();
  bool *z = Out.cpu<bool>();
  const auto *x = X.cpu<DType>();
  const auto *y = Y.cpu<DType>();

  if (x_size == y_size) {
    for (int i = 0; i < x_size; ++i) {
      z[i] = CompareFunctor()(x[i], y[i]);
    }
  } else {
    int axis = (broadcast_axis == -1 ? abs(static_cast<int>(x_dims.size()) -
                                       static_cast<int>(y_dims.size()))
                                 : broadcast_axis);
    // If Y contains only one data, all_broad_cast mode will be applied.
    // In this mode, each member in X will compare to the only var in Y.
    if (Y.numel() == 1) {
      axis = x_dims.size();
    }
    int outer_num, mid_num, inner_num;
    int is_run_common_broadcast = 0;
    int axis_trim = 0;
    auto y_dims_trimed = trim_trailing_singular_dims(y_dims);
    axis_trim = (y_dims_trimed.size() == 0) ? x_dims.size() : axis;
    get_mid_dims(x_dims,
                 y_dims_trimed,
                 axis_trim,
                 &outer_num,
                 &mid_num,
                 &inner_num,
                 &is_run_common_broadcast);
    if (is_run_common_broadcast == 1) {
      CommonElementwiseBroadcastForward<CompareFunctor, DType, bool>(
          x, y, z, x_dims, y_dims, Out.dims(), CompareFunctor());
      return;
    }

    // get_mid_dims(x_dims, y_dims, axis, &outer_num, &mid_num, &inner_num);
    for (int outer_id = 0; outer_id < outer_num; ++outer_id) {
        for (int mid_id = 0; mid_id < mid_num; ++mid_id) {
            auto y_data = y[mid_id];
            for (int inner_id = 0; inner_id < inner_num; ++inner_id) {
                int index = (outer_id * mid_num + mid_id) * inner_num + inner_id;
                z[index] = CompareFunctor()(x[index], y_data);
            }
        }
    }
  }
}
}  // namespace math
}  // namespace eagleeye