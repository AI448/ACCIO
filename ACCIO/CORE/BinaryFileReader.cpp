#include <algorithm>
#include <cassert>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "BinaryFileReader.hpp"


namespace ACCIO::CORE
{

  class BinaryFDReader: public BinaryReader
  {
  protected:

    static constexpr std::size_t default_min_buffer_size = 1024;

    int fd_;

  public:

    explicit BinaryFDReader(int fd):
      fd_(fd)
    {}

    std::size_t min_buffer_size() const noexcept override
    {
      if(fd_ < 0) return 0;
      struct stat st;
      auto ret = ::fstat(fd_, &st);
      if(ret == 0 && st.st_blksize > 0){
        // default_min_buffer_size を st.st_blksize の倍数に切り上げる．
        return ((default_min_buffer_size - 1) / static_cast<std::size_t>(st.st_blksize) + 1) * static_cast<std::size_t>(st.st_blksize);
      }else{
        // note: fstat のエラーは無視する．
        return default_min_buffer_size;
      }
    }

    std::size_t operator()(char* buffer, std::size_t limit) override
    {
      if(fd_ < 0) return 0;
      // limit を default_min_buffer_size の倍数に切り下げる．
      limit = limit / default_min_buffer_size * default_min_buffer_size;
      auto result = ::read(fd_, buffer, limit);
      if(result < 0) throw std::runtime_error("read() failure.");
      return result;
    }

    void close() noexcept override
    {
      // 何もしない．
    }

  };


  class BinaryFileReader: public BinaryFDReader
  {
  private:

    static int open(const std::string& path)
    {
      int fd = ::open(path.c_str(), O_RDONLY);
      if(fd < 0) throw std::runtime_error("Cannot open \"" + path + "\".");
      return fd;
    }

  public:

    explicit BinaryFileReader(const std::string& path):
      BinaryFDReader(open(path))
    {}

    BinaryFileReader(BinaryFileReader&& other) = default;

    ~BinaryFileReader() noexcept
    {
      close();
    }

    void close() noexcept override
    {
      if(fd_ >= 0){
        auto ret = ::close(fd_);
        // note: close のエラーはデバッグ時のみ捕捉する．
        assert(ret == 0); static_cast<void>(ret);
        fd_ = -1;
      }
    }

  };

  std::unique_ptr<BinaryReader> make_binary_fd_reader(int file_descriptor)
  {
    return std::make_unique<BinaryFDReader>(file_descriptor);
  }

  std::unique_ptr<BinaryReader> make_binary_file_reader(const std::string& file_path)
  {
    return std::make_unique<BinaryFileReader>(file_path);
  }

  std::unique_ptr<BinaryReader> make_binary_stdin_reader()
  {
    return make_binary_fd_reader(0);
  }

}
