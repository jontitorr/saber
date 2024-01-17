#ifndef SABER_RESULT_HPP
#define SABER_RESULT_HPP

#include <boost/blank.hpp>
#include <boost/outcome/result.hpp>
#include <boost/outcome/try.hpp>

namespace saber {
namespace outcome = boost::outcome_v2;
template <typename T = boost::blank>
using Result = outcome::result<T>;
#define SABER_TRY BOOST_OUTCOME_TRY
}  // namespace saber

#endif	// SABER_RESULT_HPP
