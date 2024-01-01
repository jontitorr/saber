#ifndef SABER_RESULT_HPP
#define SABER_RESULT_HPP

#include <boost/blank.hpp>
#include <boost/outcome/result.hpp>

namespace saber {
namespace outcome = boost::outcome_v2;
template <typename T = boost::blank>
using Result = outcome::result<T>;
}  // namespace saber

#endif	// SABER_RESULT_HPP
