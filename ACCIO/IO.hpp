#ifndef ACCIO_IO_HPP_
#define ACCIO_IO_HPP_


#include "CORE/BinaryFileReader.hpp"
#include "CORE/Decoder.hpp"
#include "CORE/InputStream.hpp"


namespace ACCIO
{

  struct InputMode {} IN;

  template<class CharT>
  CORE::InputStream<CharT> open(const std::string& file_path, InputMode, const std::string& encoding = "ascii")
  {
    return CORE::InputStream<CharT>(CORE::make_decoder<CharT>(CORE::make_binary_file_reader(file_path), encoding));
  }

  template<class CharT>
  CORE::InputStream<CharT> stdin(const std::string& encoding = "ascii")
  {
    return CORE::InputStream<CharT>(CORE::make_decoder<CharT>(CORE::make_binary_stdin_reader(), encoding));
  }

}


#endif
