#ifndef ACCIO_PARSECSV_HPP_
#define ACCIO_PARSECSV_HPP_


#include "CORE/CSVParser.hpp"
#include <iterator>
#include <istream>


namespace ACCIO
{

  template<class InputT>
  decltype(auto) parse_csv(InputT&& input, char8_t delimiter = ',')
  {
    using std::begin;
    using char_type = std::remove_cv_t<std::remove_reference_t<decltype(*begin(std::declval<InputT&>()))>>;
    return CORE::CSVParser<char_type>(std::forward<InputT>(input), delimiter);
  }

}


#endif
