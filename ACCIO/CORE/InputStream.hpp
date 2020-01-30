#ifndef ACCIO_CORE_INPUTSTREAM_HPP_
#define ACCIO_CORE_INPUTSTREAM_HPP_


#include "Reader.hpp"
#include <cassert>


namespace ACCIO::CORE
{

  template<class CharT>
  class InputStream
  {
  public:

    using char_type = CharT;

  private:

    static constexpr std::size_t default_min_buffer_size = 1024;
  
    // note: InputStream がローカル変数ではない状況を加味するとfirst_, last_ は Iterator に持たせたほうがパフォーマンス的にいいかもしれないが，
    // EOF まで読まずにイテレータを破棄した際にイテレータの状態を InputStream に戻すのが面倒．
    std::unique_ptr<Reader<char_type>> reader_;
    std::size_t buffer_size_;
    std::unique_ptr<char_type[]> buffer_;
    const char_type* first_;
    const char_type* last_;

  public:

    explicit InputStream(std::unique_ptr<Reader<char_type>>&& reader):
      reader_(std::move(reader)), buffer_size_(0), buffer_(), first_(nullptr), last_(nullptr)
    {
      if(reader_ != nullptr){
        buffer_size_ = std::max(reader_->min_buffer_size(), default_min_buffer_size);
        buffer_ = std::make_unique<char_type[]>(buffer_size_);
        auto n = (*reader_)(buffer_.get(), buffer_size_);
        first_ = buffer_.get();
        last_ = first_ + n;      
      }
    }

    InputStream(InputStream&& other) noexcept:
      reader_(std::move(other.reader_)), buffer_size_(other.buffer_size_), buffer_(std::move(other.buffer_)), first_(other.first_), last_(other.last_)
    {
      assert(other.reader_ == nullptr);
      assert(other.buffer_ == nullptr);
      other.buffer_size_ = 0;
      other.first_ = nullptr;
      other.last_ = nullptr;
    }

    std::size_t buffer_size() const noexcept
    {
      return buffer_size_;
    }

    bool eof() const noexcept
    {
      return first_ == last_;
    }

    const char_type& get() const noexcept
    {
      assert(!eof());
      return *first_;
    }

    void next()
    {
      assert(!eof());
      ++first_;
      if(first_ == last_){
        auto n = (*reader_)(buffer_.get(), buffer_size_);
        first_ = buffer_.get();
        last_ = first_ + n;      
      }
    }

    class LastIterator;

    class Iterator
    {
    private:

      InputStream& stream_;

    public:

      explicit Iterator(InputStream& stream):
        stream_(stream)
      {}

      Iterator(Iterator&&) = default;

      bool operator==(const LastIterator&) const noexcept
      {
        return stream_.eof();
      }

      bool operator!=(const LastIterator&) const noexcept
      {
        return !stream_.eof();
      }

      const char_type& operator*() const noexcept
      {
        return stream_.get();
      }

      Iterator& operator++()
      {
        stream_.next();
        return *this;
      }

    // deleted:

      Iterator() = delete;
      Iterator(const Iterator&) = delete;
      Iterator& operator=(Iterator&&) = delete;
      Iterator& operator=(const Iterator&) = delete;      

    };

    class LastIterator
    {
    public:

      bool operator==(const Iterator& rhs) const noexcept
      {
        return rhs == *this;
      }

      bool operator!=(const Iterator& rhs) const noexcept
      {
        return rhs != *this;
      }

    };

    Iterator begin() noexcept
    {
      return Iterator(*this);
    }

    LastIterator end() noexcept
    {
      return {};
    }

  // deleted:

    InputStream() = delete;
    InputStream(const InputStream&) = delete;
    InputStream& operator=(InputStream&&) = delete;
    InputStream& operator=(const InputStream&) = delete;

  };

  
}


#endif
