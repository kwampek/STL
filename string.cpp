#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

class String {
 private:
  size_t size_;
  size_t cap_;
  char* string_;

  size_t finds_helper(const String& substring, bool isForwardFind) const;
  void set_capacity(size_t new_capacity);
  void try_extend();
  explicit String(size_t num);

 public:
  String();
  String(const char* another_str);
  String(size_t num, char chr);
  String(const String& another_str);

  ~String();

  void swap(String& another);

  String& operator=(const String& another);

  char& operator[](size_t idx);
  const char& operator[](size_t idx) const;

  size_t length() const;
  size_t capacity() const;
  size_t size() const;

  char& front();
  const char& front() const;
  char& back();
  const char& back() const;

  String& operator+=(const String& another);
  String& operator+=(char chr);

  void push_back(char chr);
  void pop_back();

  size_t find(const String& substring) const;
  size_t rfind(const String& substring) const;
  String substr(size_t start, size_t count) const;

  bool empty() const;
  void clear();
  void shrink_to_fit();
  char* data();
  const char* data() const;
};

String::String() : size_(0), cap_(0), string_(nullptr) {}

String::String(size_t num, char chr) : String(num) {
  std::fill(string_, string_ + num, chr);
}

String::String(const String& another_str)
    : size_(another_str.size_),
      cap_(another_str.cap_),
      string_(new char[another_str.cap_ + 1]) {
  std::copy(another_str.string_, another_str.string_ + size_ + 1, string_);
}

String::String(const char* another_str) : String(strlen(another_str)) {
  std::copy(another_str, another_str + size_, string_);
}

String::String(size_t num) : size_(num), cap_(num), string_(new char[num + 1]) {
  string_[size_] = '\0';
}

void String::set_capacity(size_t new_capacity) {
  char* new_string = new char[new_capacity + 1];
  size_ = std::min(size_, new_capacity);
  std::copy(string_, string_ + size_, new_string);
  new_string[size_] = '\0';

  delete[] string_;
  cap_ = new_capacity;
  string_ = new_string;
}

bool operator==(const String& one, const String& another) {
  return one.size() == another.size() &&
         memcmp(one.data(), another.data(), one.size()) == 0;
}

bool operator!=(const String& one, const String& another) {
  return !(one == another);
}

bool operator<(const String& one, const String& another) {
  int cmp_result = memcmp(one.data(), another.data(),
                          std::min(one.size(), another.size()) + 1);
  return (cmp_result == 0 && one.size() < another.size()) || (cmp_result < 0);
}
bool operator>(const String& one, const String& another) {
  return another < one;
}
bool operator<=(const String& one, const String& another) {
  return !(one > another);
}
bool operator>=(const String& one, const String& another) {
  return !(one < another);
}

void String::swap(String& another) {
  std::swap(size_, another.size_);
  std::swap(cap_, another.cap_);
  std::swap(string_, another.string_);
}

String& String::operator=(const String& another) {
  if (this == &another) {
    return *this;
  }

  if (cap_ < another.size_) {
    set_capacity(another.size_);
  }

  std::copy(another.data(), another.data() + another.size_, string_);
  size_ = another.size_;
  return *this;
}

void String::push_back(char chr) {
  try_extend();

  string_[size_] = chr;
  string_[++size_] = '\0';
}

void String::pop_back() { string_[--size_] = '\0'; }

std::ostream& operator<<(std::ostream& out, const String& string) {
  for (size_t ind = 0; ind < string.size(); ++ind) {
    out << string[ind];
  }
  return out;
}

std::istream& operator>>(std::istream& input, String& string) {
  string.clear();
  char chr;
  bool isCharReceived = false;
  while (input.get(chr)) {
    if (isspace(chr) != 0) {
      if (isCharReceived) {
        break;
      }
    } else {
      isCharReceived = true;
      string.push_back(chr);
    }
  }
  return input;
}

String operator+(const String& string, const String& another_str) {
  size_t str_size = string.size();
  String copy(str_size + another_str.size(), '\0');
  std::copy(string.data(), string.data() + str_size, copy.data());
  std::copy(another_str.data(), another_str.data() + another_str.size(),
            copy.data() + str_size);
  return copy;
}

String operator+(char chr, const String& another_str) {
  String copy(another_str.size() + 1, '\0');
  copy[0] = chr;
  std::copy(another_str.data(), another_str.data() + another_str.size(),
            copy.data() + 1);
  return copy;
}

String operator+(const String& string, char chr) {
  String copy(string.size() + 1, '\0');
  std::copy(string.data(), string.data() + string.size(), copy.data());
  copy[string.size()] = chr;
  return copy;
}

String& String::operator+=(const String& another) {
  set_capacity(size_ + another.size_ + 1);
  std::copy(another.string_, another.string_ + another.size_ + 1,
            string_ + size_);
  size_ += another.size_;
  return *this;
}

String& String::operator+=(char chr) {
  push_back(chr);
  return *this;
}

size_t String::finds_helper(const String& substring, bool isForwardFind) const {
  if (substring.size_ > size_) {
    return size_;
  }

  size_t end = isForwardFind ? size_ - substring.size_ + 1 : -1;
  size_t begin = isForwardFind ? 0 : size_ - substring.size_;

  for (size_t i = begin; i != end; isForwardFind ? ++i : --i) {
    if (memcmp(string_ + i, substring.string_, substring.size_) == 0) {
      return i;
    }
  }
  return size_;
}

size_t String::find(const String& substring) const {
  return finds_helper(substring, true);
}

size_t String::rfind(const String& substring) const {
  return finds_helper(substring, false);
}

String String::substr(size_t start, size_t count) const {
  count = std::min(size_ - start, count);
  String copy(count);
  std::copy(string_ + start, string_ + start + count, copy.string_);
  copy[count] = '\0';
  return copy;
}

void String::shrink_to_fit() {
  if (size_ != cap_) {
    set_capacity(size_);
  }
}

bool String::empty() const { return size_ == 0; }
void String::clear() {
  if (size_ != 0) {
    size_ = 0;
    string_[size_] = '\0';
  }
}
const char* String::data() const { return string_; }

char& String::front() { return string_[0]; }
const char& String::front() const { return string_[0]; }
char& String::back() { return string_[size_ - 1]; }
const char& String::back() const { return string_[size_ - 1]; }

char& String::operator[](size_t idx) { return string_[idx]; }
const char& String::operator[](size_t idx) const { return string_[idx]; }

char* String::data() { return string_; }
size_t String::length() const { return size_; }
size_t String::capacity() const { return cap_; }
size_t String::size() const { return size_; }

String::~String() { delete[] string_; }

void String::try_extend() {
  if ((size_ + 1) >= cap_) {
    set_capacity(cap_ * 2 + 1);
  }
}
