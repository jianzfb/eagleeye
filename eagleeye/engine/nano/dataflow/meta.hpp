#ifndef _EAGLEEYE_META_H_
#define _EAGLEEYE_META_H_
#include "eagleeye/common/EagleeyeMacro.h"
namespace eagleeye{
namespace dataflow {
template <std::size_t ... Is>
struct index_sequence {};

namespace impl {

template <std::size_t N, std::size_t ... Is>
struct make_index_sequence {
  using type = typename make_index_sequence<N - 1, N - 1, Is ...>::type;
};

template <std::size_t ... Is>
struct make_index_sequence<0, Is ...> {
  using type = index_sequence<Is ...>;
};
}

template <std::size_t N>
using make_index_sequence = typename impl::make_index_sequence<N>::type;

}

}
#endif

