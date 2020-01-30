#include "fileio.hpp"
#include "parse_csv.hpp"
#include <iostream>


int main(int argc, char* argv[])
{
  using namespace ACCIO;
  std::size_t rows = 0;
  std::size_t elements = 0;
  for(auto&& record: parse_csv(open<char8_t>(argv[1], ACCIO::in, "utf-8"))){
    ++rows;
    for(auto&& field: record){
      std::cout << field << '\t';
      ++elements;
    }
    std::cout << std::endl;
  }
  std::cout << rows << " rows, " << elements << " elements." << std::endl;
  return 0;
}
