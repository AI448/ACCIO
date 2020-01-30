#ifndef ACCIO_CORE_DECORDER_HPP_
#define ACCIO_CORE_DECORDER_HPP_


#include "Reader.hpp"


namespace ACCIO::CORE
{

  template<class CharT,
    std::enable_if_t<std::is_same_v<CharT, char8_t>>* = nullptr>
  std::unique_ptr<Reader<CharT>> make_decoder(std::unique_ptr<BinaryReader>&& binary_reader, const std::string& encoding);

  template<class CharT,
    std::enable_if_t<std::is_same_v<CharT, char8_t>>* = nullptr>
  std::unique_ptr<Reader<CharT>> make_decoder(std::shared_ptr<BinaryReader>& binary_reader, const std::string& encoding);

}


#endif
