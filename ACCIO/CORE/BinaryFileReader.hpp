#ifndef ACCIO_CORE_BINARYFILERADER_HPP_
#define ACCIO_CORE_BINARYFILERADER_HPP_


#include "Reader.hpp"


namespace ACCIO::CORE
{

  std::unique_ptr<BinaryReader> make_binary_fd_reader(int file_descriptor);

  std::unique_ptr<BinaryReader> make_binary_file_reader(const std::string& file_path);

  std::unique_ptr<BinaryReader> make_binary_stdin_reader();

}


#endif
