#include "fileio.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
  using namespace ACCIO;
  try{
    for(auto c: open<char8_t>(argv[1], ACCIO::in, "utf-8")){
      std::cout << c;
    }
    std::cout << std::flush;
  }catch(const std::exception& exc){
    std::cout << std::flush;
    std::cerr << exc.what() << std::endl;
  }

  return 0;
}

