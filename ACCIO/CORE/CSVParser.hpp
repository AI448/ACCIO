#ifndef ACCIO_CORE_CSVPARSER_HPP_
#define ACCIO_CORE_CSVPARSER_HPP_


#include <cassert>
#include <cstdint>
#include <string_view>
#include <stdexcept>
#include <vector>
#include "char8_t.hpp"


namespace ACCIO::CORE
{

//  using std::begin;
//  using std::end;

  template<class charT>
  class CSVParser;

  template<class CharT>
  class CSVRecord
  {
    template<class>
    friend class CSVParser;

  public:

    using char_type = CharT;
    using string_view = std::basic_string_view<char_type>;

  private:

    struct FieldInfo
    {
      std::size_t position_;
      std::size_t length_;

      FieldInfo(std::size_t position, std::size_t length):
        position_(position), length_(length)
      {}

      FieldInfo(FieldInfo&&) = default;
      FieldInfo(const FieldInfo&) = default;
      FieldInfo& operator=(FieldInfo&&) = default;
      FieldInfo& operator=(const FieldInfo&) = default;

    // deleted:
  
      FieldInfo() = delete;
  
    };

    std::vector<FieldInfo> field_infos_;
    std::vector<char_type> text_;

  public:

    /// record の要素数を返す．
    std::size_t size() const noexcept
    {
      return field_infos_.size();
    }

    /// i 番目の要素を返す．
    string_view operator[](std::size_t i) const noexcept
    {
      assert(i < field_infos_.size());
      return string_view(text_.data() + field_infos_[i].position_, field_infos_[i].length_);
    }

    class Iterator
    {
    private:

      const CSVRecord& record_;
      std::size_t index_;

    public:

      Iterator(const CSVRecord& record, std::size_t index) noexcept:
        record_(record), index_(index)
      {}

      Iterator(Iterator&&) = default;
      Iterator(const Iterator&) = default;

      bool operator==(const Iterator& rhs) const noexcept
      {
        return index_ == rhs.index_;
      }

      bool operator!=(const Iterator& rhs) const noexcept
      {
        return !operator==(rhs);
      }

      decltype(auto) operator*() const noexcept
      {
        return record_[index_];
      }

      Iterator& operator++() noexcept
      {
        ++index_;
        return *this;
      }
    
    };

    Iterator begin() const noexcept
    {
      return Iterator(*this, 0);
    }

    Iterator end() const noexcept
    {
      return Iterator(*this, field_infos_.size());
    }

  };

  template<class CharT>
  class CSVParser
  {
  public:

    using char_type = CharT;
    using Record = CSVRecord<char_type>;

    static constexpr char_type double_quate = '\"';
    static constexpr char_type line_feed = '\n';
    static constexpr char_type carriage_return = '\r';

  private:

    class ImplBase
    {
    protected:

      ImplBase() = default;
      ImplBase(ImplBase&&) = default;
      ImplBase(const ImplBase&) = default;

    public:

      virtual ~ImplBase() = default;

      virtual bool eof() const noexcept = 0;

      virtual const Record& get() const noexcept = 0;

      virtual void next() = 0;

    };

    template<class IteratorT, class LastIteratorT>
    class Impl: public ImplBase
    {
    private:

      Record buffer_;
      char_type delimiter_;
      IteratorT current_;
      LastIteratorT last_;

    public:

      template<class T, class U>
      Impl(T&& iterator, U&& last_iterator, char_type delimiter) noexcept:
        ImplBase(), buffer_(), delimiter_(delimiter), current_(std::forward<T>(iterator)), last_(std::forward<U>(last_iterator))
      {}

      bool eof() const noexcept override
      {
        return current_ == last_;
      }

      const Record& get() const noexcept override
      {
        return buffer_;
      }

      void next() override
      {
        assert(current_ != last_);
        buffer_.field_infos_.clear();
        buffer_.text_.clear();
        // Start of Record
        while(true){
          // Start of Field
          buffer_.field_infos_.emplace_back(buffer_.text_.size(), 0);
          if(current_ == last_){
            break;
          }else if(*current_ != double_quate){
            // In Field
            while(current_ != last_){
              if(*current_ == delimiter_ || *current_ == line_feed){
                break;
              }else if(*current_ == carriage_return){
                ++current_;
                // Next of CR
                if(current_ != last_ && *current_ == line_feed){
                  break;
                }else{
                  buffer_.text_.push_back(carriage_return);
                  ++(buffer_.field_infos_.back().length_);
                }
              }else if(*current_ == double_quate){
                throw std::runtime_error("unexpected double quate");
              }else{
                buffer_.text_.push_back(*current_);
                ++(buffer_.field_infos_.back().length_);
                ++current_;
              }
            }
          }else{
            assert(*current_ == double_quate);
            ++current_;
            // In Enclosed Field
            while(true){
              if(current_ == last_){
                throw std::runtime_error("unexpected EOF");
              }else if(*current_ == double_quate){
                ++current_;
                // Next of Double Quote
                if(current_ == last_ || *current_ == delimiter_ || *current_ == line_feed){
                  break;
                }else if(*current_ == carriage_return){
                  ++current_;
                  // Next of CR
                  if(current_ != last_ && *current_ == line_feed){
                    break;
                  }else{
                    throw std::runtime_error("hoge");
                  }
                }else if(*current_ == double_quate){
                  buffer_.text_.push_back(double_quate);
                  ++(buffer_.field_infos_.back().length_);
                  ++current_;
                }else{
                  throw std::runtime_error("foo");
                }
              }else{
                buffer_.text_.push_back(*current_);
                ++(buffer_.field_infos_.back().length_);
                ++current_;
              }
            }
          }
          assert(current_ == last_ || *current_ == delimiter_ || *current_ == line_feed);
          // End of Field
          buffer_.text_.push_back('\0');
          if(current_ == last_){
            break;
          }else if(*current_ == line_feed){
            ++current_;
            break;
          }else{
            ++current_;
          }
        }
        // End of Record
      }

    };

    template<class InputStreamT>
    class Capturer
    {
    protected:

      InputStreamT input_stream_;

      Capturer(InputStreamT&& input_stream):
        input_stream_(std::move(input_stream))
      {}

    };

    // note: 先にキャプチャしてからイテレータを取得する必要があるため，多重継承を使ったトリッキーな実装となっている．
    template<class InputStreamT>
    class ImplWithCapturing:
      public Capturer<InputStreamT>,
      public Impl<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<InputStreamT&>().begin())>>,
                  std::remove_cv_t<std::remove_reference_t<decltype(std::declval<InputStreamT&>().end())>>>
    {
    public:

      ImplWithCapturing(InputStreamT&& input_stream, char_type delimiter):
        Capturer<InputStreamT>(std::move(input_stream)),
        Impl<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<InputStreamT&>().begin())>>,
             std::remove_cv_t<std::remove_reference_t<decltype(std::declval<InputStreamT&>().end())>>>(
               std::begin(Capturer<InputStreamT>::input_stream_), std::end(Capturer<InputStreamT>::input_stream_), delimiter)
      {}
    };

    class LastIterator;

    class Iterator
    {
    private:

      CSVParser& parser_;

    public:

      explicit Iterator(CSVParser& parser):
        parser_(parser)
      {}

      Iterator(Iterator&&) = default;

      bool operator==(const LastIterator&) const noexcept
      {
        return parser_.impl_ == nullptr;
      }

      bool operator!=(const LastIterator&) const noexcept
      {
        return parser_.impl_ != nullptr;
      }

      decltype(auto) operator*() const noexcept
      {
        assert(parser_.impl_ != nullptr);
        return parser_.impl_->get();
      }

      Iterator& operator++()
      {
        assert(parser_.impl_ != nullptr);
        if(parser_.impl_->eof()){
          parser_.impl_ = nullptr;
        }else{
          parser_.impl_->next();
        }
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


    std::unique_ptr<ImplBase> impl_;

  public:

    template<class InputStreamT>
    CSVParser(InputStreamT&& stream, char_type delimiter):
      impl_(std::make_unique<ImplWithCapturing<InputStreamT>>(std::forward<InputStreamT>(stream), delimiter))
    {
      if(impl_->eof()){
        impl_ = nullptr;
      }else{
        impl_->next();
      }
    }

    template<class IteratorT, class LastIteratorT>
    CSVParser(IteratorT&& first, LastIteratorT&& last, char_type delimiter):
      impl_(std::make_unique<Impl<std::remove_cv_t<std::remove_reference_t<IteratorT>>, std::remove_cv_t<std::remove_reference_t<LastIteratorT>>>>(
        std::forward<IteratorT>(first), std::forward<LastIteratorT>(last), delimiter))
    {
      if(impl_->eof()){
        impl_ = nullptr;
      }else{
        impl_->next();
      }
    }

    Iterator begin() noexcept
    {
      return Iterator(*this);
    }

    LastIterator end() noexcept
    {
      return {};
    }

  };

}

#endif
