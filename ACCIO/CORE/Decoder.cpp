#include <cassert>
#include "Decoder.hpp"


namespace ACCIO::CORE
{

  static std::tuple<bool, unsigned> parse_u8char(const char* s) noexcept
  {
    const std::uint8_t* t = reinterpret_cast<const std::uint8_t*>(s);
    if(t[0] <= 0x7F){
      return {true, 1};
    }else if(0xC2 <= t[0] && t[0] <= 0xDF){
      if(0x80 <= t[1] && t[1] <= 0xBF) return {true, 2};
      else return {false, 2};
    }else if(t[0] == 0xE0){
      if(0xA0 <= t[1] && t[1] <= 0xBF){
        if(0x80 <= t[2] && t[2] <= 0xBF) return {true, 3};
        else return {false, 3};
      }else return {false, 2};
    }else if(0xE1 <= t[0] && t[0] <= 0xEC){
      if(0x80 <= t[1] && t[1] <= 0xBF){
        if(0x80 <= t[2] && t[2] <= 0xBF) return {true, 3};
        else return {false, 3};
      }else return {false, 2};
    }else if(t[0] == 0xED){
      if(0x80 <= t[1] && t[1] <= 0x9F){
        if(0x80 <= t[2] && t[2] <= 0xBF) return {true, 3};
        else return {false, 3};
      }else return {false, 2};
    }else if(0xEE <= t[0] && t[0] <= 0xEF){
      if(0x80 <= t[1] && t[1] <= 0xBF){
        if(0x80 <= t[2] && t[2] <= 0xBF) return {true, 3};
        else return {false, 3};
      }else return {false, 2};
    }else if(t[0] == 0xF0){
      if(0x90 <= t[1] && t[1] <= 0xBF){
        if(0x80 <= t[2] && t[2] <= 0xBF){
          if(0x80 <= t[3] && t[3] <= 0xBF) return {true, 4};
          else return {false, 4}; 
        }else return {false, 3};
      }else return {false, 2};
    }else if(0xF1 <= t[0] && t[0] <= 0xF3){
      if(0x80 <= t[1] && t[1] <= 0xBF){
        if(0x80 <= t[2] && t[2] <= 0xBF){
          if(0x80 <= t[3] && t[3] <= 0xBF) return {true, 4};
          else return {false, 4};
        }else return {false, 3};
      }else return {false, 2};
    }else if(t[0] == 0xF4){
      if(0x80 <= t[1] && t[1] <= 0x8F){
        if(0x80 <= t[2] && t[2] <= 0xBF){
          if(0x80 <= t[3] && t[3] <= 0xBF) return {true, 4};
          else return {false, 4};
        }else return {false, 3};
      }else return {false, 2};
    }else return {false, 1};
  }


  template<class BinaryReaderPtrT>
  class U8DecoderFromAscii: public U8Reader
  {
    static_assert(std::is_same_v<BinaryReaderPtrT, std::unique_ptr<BinaryReader>> || std::is_same_v<BinaryReaderPtrT, std::shared_ptr<BinaryReader>>);
  private:

    BinaryReaderPtrT binary_reader_;

  public:

    template<class T>
    explicit U8DecoderFromAscii(T&& binary_reader):
      binary_reader_(std::forward<T>(binary_reader))
    {}

    std::size_t min_buffer_size() const noexcept override
    {
      if(binary_reader_ != nullptr){
        return binary_reader_->min_buffer_size();
      }else{
        return 0;
      }
    }

    std::size_t operator()(char_type* buffer, std::size_t limit) override
    {
      if(binary_reader_ != nullptr){
        auto n = (*binary_reader_)(reinterpret_cast<char*>(buffer), limit);
        for(std::size_t i = 0; i < n; ++i){
          if(static_cast<std::uint8_t>(buffer[i]) & 0x80) throw std::runtime_error("not ascii.");
        }
        return n;
      }else{
        return 0;
      }
    }

    void close() noexcept override
    {
      binary_reader_ = nullptr;
    }

  };

  template<class BinaryReaderPtrT>
  class U8DecoderFromUTF8: public U8Reader
  {
    static_assert(std::is_same_v<BinaryReaderPtrT, std::unique_ptr<BinaryReader>> || std::is_same_v<BinaryReaderPtrT, std::shared_ptr<BinaryReader>>);

  private:

    BinaryReaderPtrT binary_reader_;
    std::size_t min_buffer_size_;
    std::size_t frag_size_;
    char_type frag_buffer_[4];

  public:

    template<class T>
    explicit U8DecoderFromUTF8(T&& binary_reader):
      binary_reader_(std::forward<T>(binary_reader)), min_buffer_size_(binary_reader_->min_buffer_size() + 6), frag_size_(0), frag_buffer_()
    {}

    std::size_t min_buffer_size() const noexcept override
    {
      return min_buffer_size_;
    }

    std::size_t operator()(char_type* buffer, std::size_t limit) override
    {
      if(binary_reader_ != nullptr){
        assert(limit >= min_buffer_size_);
        for(std::uint8_t i = 0; i < frag_size_; ++i){
          buffer[i] = frag_buffer_[i];
        }
        //
        auto m = (*binary_reader_)(reinterpret_cast<char*>(buffer) + frag_size_, limit - frag_size_ - 3);
        auto n = m + frag_size_;
        frag_size_ = 0;
        //
        if(n == 0) return 0;
        //
        assert(n + 3 <= limit);
        buffer[n] = 0;
        buffer[n + 1] = 0;
        buffer[n + 2] = 0;
        //
        auto [is_valid, bytes] = parse_u8char(buffer);
        if(!is_valid) throw std::runtime_error("Invalid encoding");
        //
        assert(bytes <= n);
        for(std::size_t i = bytes; i < n;){
          auto [is_valid, bytes] = parse_u8char(buffer + i);
          if(is_valid){
            i += bytes;
            assert(i <= n);
          }else{
            frag_size_ = std::min<std::size_t>(bytes, n - i);
            for(std::size_t j = 0; j < frag_size_; ++j){
              frag_buffer_[j] = buffer[i + j];
            }
            return i;
          }
        }
        return n;
      }else{
        return 0;
      }
    }

    void close() noexcept override
    {
      binary_reader_ = nullptr;
      min_buffer_size_ = 0;
      frag_size_ = 0;
    }

  };

  // utf-8 への変換
  template<class CharT, class BinaryReaderPtrT,
    std::enable_if_t<std::is_same_v<CharT, char8_t>>* = nullptr>
  std::unique_ptr<Reader<CharT>> make_decoder_impl(BinaryReaderPtrT&& binary_reader, const std::string& encoding)
  {
    if(encoding == "ascii"){
      return std::make_unique<U8DecoderFromAscii<std::remove_reference_t<BinaryReaderPtrT>>>(std::forward<BinaryReaderPtrT>(binary_reader));
    }else if(encoding == "utf-8"){
      return std::make_unique<U8DecoderFromUTF8<std::remove_reference_t<BinaryReaderPtrT>>>(std::forward<BinaryReaderPtrT>(binary_reader));
    }else{
      throw std::runtime_error("not implemented.");
    }
  }

  template<class CharT,
    std::enable_if_t<std::is_same_v<CharT, char8_t>>* = nullptr>
  std::unique_ptr<Reader<CharT>> make_decoder(std::unique_ptr<BinaryReader>&& binary_reader, const std::string& encoding)
  {
    return make_decoder_impl<CharT>(std::move(binary_reader), encoding);
  }

  template<class CharT,
    std::enable_if_t<std::is_same_v<CharT, char8_t>>* = nullptr>
  std::unique_ptr<Reader<CharT>> make_decoder(std::shared_ptr<BinaryReader>& binary_reader, const std::string& encoding)
  {
    return make_decoder_impl<CharT>(binary_reader, encoding);
  }

  // 明示的な実体化

  template std::unique_ptr<Reader<char8_t>> make_decoder<char8_t>(std::unique_ptr<BinaryReader>&& binary_reader, const std::string& encoding);

  template std::unique_ptr<Reader<char8_t>> make_decoder<char8_t>(std::shared_ptr<BinaryReader>& binary_reader, const std::string& encoding);

}
