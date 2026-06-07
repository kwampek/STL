#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

class BigInteger;
bool operator<(const BigInteger& one, const BigInteger& another);
bool operator==(const BigInteger& one, const BigInteger& another);
bool operator>(const BigInteger& one, const BigInteger& another);
bool operator<=(const BigInteger& one, const BigInteger& another);
bool operator>=(const BigInteger& one, const BigInteger& another);
bool operator!=(const BigInteger& one, const BigInteger& another);

class BigInteger {
 private:
  bool is_positive_or_null = true;
  std::vector<uint32_t> digits_;
  void toNormalView();
  BigInteger conditionsForMinus(const BigInteger& another);

  static const uint32_t kDegree = 6;
  static const uint32_t kBase = 1'000'000;

 public:
  BigInteger() : digits_({0}) {}

  BigInteger(int number);
  BigInteger(std::string string);

  BigInteger operator+=(const BigInteger& another);
  BigInteger operator-=(const BigInteger& another);
  BigInteger operator*=(const BigInteger& another);
  BigInteger operator/=(const BigInteger& another);
  BigInteger operator%=(const BigInteger& another);

  BigInteger operator-() const;

  bool isPositiveOrNull() const;
  void invertSign();

  explicit operator bool() const;

  std::string toString() const;
  size_t firstNotEq(const BigInteger& another) const;
  BigInteger byPowerOfTen(size_t power) const;
  uint32_t operator[](size_t index) const;

  BigInteger& operator++();
  BigInteger operator++(int);
  BigInteger& operator--();
  BigInteger operator--(int);

  size_t size() const;
  const std::vector<uint32_t>& getData() const;

  bool isNull() const;

  static uint32_t getDegree();
  static uint32_t getBase();
};

bool BigInteger::isPositiveOrNull() const { return is_positive_or_null; }
size_t BigInteger::size() const { return digits_.size(); }

BigInteger operator""_bi(const char* str) { return BigInteger(str); }

BigInteger operator""_bi(unsigned long long number) {
  return BigInteger(std::to_string(number));
}

std::ostream& operator<<(std::ostream& out, const BigInteger& big_integer) {
  if (!big_integer.isPositiveOrNull() && big_integer != 0) {
    out << "-";
  }

  std::string output;
  for (size_t i = big_integer.size() - 1; i != static_cast<size_t>(-1); --i) {
    output = std::to_string(big_integer[i]);
    if (i != big_integer.size() - 1) {
      for (size_t i = 0; i < BigInteger::getDegree() - output.size(); ++i) {
        out << "0";
      }
    }
    out << output;
  }
  return out;
}

std::istream& operator>>(std::istream& input, BigInteger& bog_integer) {
  std::string string_int;
  input >> string_int;
  bog_integer = BigInteger(string_int);
  return input;
}

BigInteger::operator bool() const { return *this != 0; }

std::string BigInteger::toString() const {
  std::string output;
  std::string answer =
      ((!is_positive_or_null && (digits_.size() != 1 || digits_[0] != 0))
           ? "-"
           : "") +
      std::to_string(digits_[size() - 1]);

  for (size_t i = size() - 2; i != static_cast<size_t>(-1); --i) {
    output = std::to_string(digits_[i]);
    answer += std::string(kDegree - output.size(), '0');
    answer += output;
  }

  return answer;
}

size_t BigInteger::firstNotEq(const BigInteger& another) const {
  for (size_t i = size() - 1; i != static_cast<size_t>(-1); --i) {
    if (digits_[i] != another[i]) {
      return i;
    }
  }
  return 0;
}

BigInteger BigInteger::byPowerOfTen(size_t power) const {
  if (*this == 0) {
    return 0;
  }
  std::string str = toString();
  str += std::string(power, '0');
  return str;
}

void BigInteger::toNormalView() {
  for (size_t i = digits_.size() - 1; i > 0; --i) {
    if (digits_[i] != 0) {
      return;
    }
    digits_.pop_back();
  }
  if (!is_positive_or_null && isNull()) {
    invertSign();
  }
}

uint32_t BigInteger::operator[](size_t index) const {
  if (index >= digits_.size()) {
    return 0;
  }
  return digits_[index];
}
void BigInteger::invertSign() { is_positive_or_null = !is_positive_or_null; }

BigInteger& BigInteger::operator++() {
  *this += 1;
  return *this;
}

BigInteger BigInteger::operator++(int) {
  BigInteger copy = *this;
  *this += 1;
  return copy;
}

BigInteger& BigInteger::operator--() {
  *this -= 1;
  return *this;
}

BigInteger BigInteger::operator--(int) {
  BigInteger copy = *this;
  *this -= 1;
  return copy;
}

BigInteger operator+(const BigInteger& one, const BigInteger& another) {
  BigInteger copy(one);
  return copy += another;
}
BigInteger operator-(const BigInteger& one, const BigInteger& another) {
  BigInteger copy(one);
  return copy -= another;
}

BigInteger operator*(const BigInteger& one, const BigInteger& another) {
  BigInteger copy(one);
  return copy *= another;
}

BigInteger operator/(const BigInteger& one, const BigInteger& another) {
  BigInteger copy(one);
  return copy /= another;
}

BigInteger operator%(const BigInteger& one, const BigInteger& another) {
  BigInteger copy(one);
  return copy %= another;
}

bool operator<(const BigInteger& one, const BigInteger& another) {
  if (one.isPositiveOrNull() != another.isPositiveOrNull()) {
    return another.isPositiveOrNull();
  }
  if (one.size() != another.size()) {
    return one.isPositiveOrNull() ? one.size() < another.size()
                                  : one.size() > another.size();
  }
  size_t first_not_equal = one.firstNotEq(another);
  return one.isPositiveOrNull()
             ? one[first_not_equal] < another[first_not_equal]
             : one[first_not_equal] > another[first_not_equal];
}

bool operator==(const BigInteger& one, const BigInteger& another) {
  return one.isPositiveOrNull() == another.isPositiveOrNull() &&
         one.getData() == another.getData();
}
bool operator>(const BigInteger& one, const BigInteger& another) {
  return another < one;
}
bool operator<=(const BigInteger& one, const BigInteger& another) {
  return !(one > another);
}
bool operator>=(const BigInteger& one, const BigInteger& another) {
  return !(one < another);
}
bool operator!=(const BigInteger& one, const BigInteger& another) {
  return !(one == another);
}

BigInteger BigInteger::operator-() const {
  BigInteger copy(*this);
  if (copy != 0) {
    copy.invertSign();
  }
  return copy;
}

BigInteger BigInteger::operator+=(const BigInteger& another) {
  size_t max_size = std::max(size(), another.size());
  if (is_positive_or_null != another.is_positive_or_null) {
    *this -= -another;
    return *this;
  }

  uint32_t carry = 0;
  for (size_t i = 0; i < max_size || carry == 1; ++i) {
    if (i >= size()) {
      digits_.push_back(0);
    }
    digits_[i] += carry + another[i];
    if (digits_[i] >= kBase) {
      digits_[i] -= kBase;
      carry = 1;
    } else {
      carry = 0;
    }
  }
  toNormalView();
  return *this;
}

BigInteger BigInteger::operator-=(const BigInteger& another) {
  if (another.isNull() ||
      (is_positive_or_null != another.is_positive_or_null) ||
      (is_positive_or_null && *this < another) || (!is_positive_or_null)) {
    return conditionsForMinus(another);
  }

  uint32_t carry = 0;
  for (size_t i = 0; i < size() || carry == 1; ++i) {
    if (digits_[i] < (another[i] + carry)) {
      digits_[i] += kBase - another[i] - carry;
      carry = 1;
    } else {
      digits_[i] -= (another[i] + carry);
      carry = 0;
    }
  }

  toNormalView();
  return *this;
}

BigInteger::BigInteger(int number) {
  if (number < 0) {
    number = -number;
    is_positive_or_null = false;
  }

  if (number == 0) {
    digits_.push_back(0);
  }

  while (number > 0) {
    digits_.push_back(number % kBase);
    number /= kBase;
  }
}

const std::vector<uint32_t>& BigInteger::getData() const { return digits_; }

BigInteger::BigInteger(std::string string_int) {
  if (string_int[0] == '-') {
    is_positive_or_null = false;
    string_int[0] = '0';
  }

  size_t remainder = string_int.size() % kDegree;
  std::string tmp_str = string_int.substr(0, remainder);
  if (remainder > 0 && tmp_str != "-") {
    digits_.push_back(abs(stoi(tmp_str)));
  }

  for (size_t index = remainder; index < string_int.size(); index += kDegree) {
    digits_.push_back(stoi(string_int.substr(index, kDegree)));
  }
  reverse(digits_.begin(), digits_.end());
}

BigInteger BigInteger::operator*=(const BigInteger& another) {
  BigInteger res;
  res.is_positive_or_null =
      (is_positive_or_null == another.is_positive_or_null);
  res.digits_.resize(size() + another.size());

  long long carry;
  long long tmp_long;
  for (size_t i = 0; i < size(); ++i) {
    carry = 0;
    for (size_t j = 0; j < another.size() || carry != 0; ++j) {
      tmp_long = res[i + j] + carry + digits_[i] * 1LL * another[j];
      carry = tmp_long / kBase;
      res.digits_[i + j] = tmp_long % kBase;
    }
  }

  *this = res;
  toNormalView();
  return *this;
}

int findNextDigit(const BigInteger& tmp, const BigInteger& another) {
  int left = 0;
  int right = BigInteger::getBase();
  int mid;
  BigInteger mult;
  while (left + 1 < right) {
    mid = (left + right) / 2;
    mult = another * mid;
    if (tmp < mult) {
      right = mid;
    } else {
      left = mid;
    }
  }
  return left;
}

BigInteger BigInteger::operator/=(const BigInteger& another) {
  if (!is_positive_or_null) {
    *this = (-*this /= another);
    invertSign();
    toNormalView();
    return *this;
  }

  if (!another.is_positive_or_null) {
    *this = (*this /= -another);
    invertSign();
    toNormalView();
    return *this;
  }

  BigInteger tmp = 0;
  BigInteger ans = 0;
  if (is_positive_or_null != another.is_positive_or_null) {
    ans.is_positive_or_null = false;
  }
  for (int i = size() - 1; i >= 0; --i) {
    tmp = tmp.byPowerOfTen(kDegree);
    tmp += digits_[i];
    if (tmp < another) {
      if (tmp == 0) {
        ans.digits_.push_back(0);
      }
      continue;
    }

    int first_bigger = findNextDigit(tmp, another);

    ans.digits_.push_back(first_bigger);
    tmp -= (another * first_bigger);
  }

  reverse(ans.digits_.begin(), ans.digits_.end());
  ans.toNormalView();

  return *this = ans;
}

BigInteger BigInteger::operator%=(const BigInteger& another) {
  return *this -= ((*this / another) * another);
}
bool BigInteger::isNull() const { return size() == 1 && digits_[0] == 0; }

BigInteger BigInteger::conditionsForMinus(const BigInteger& another) {
  if (another.isNull()) {
    return *this;
  }

  if (is_positive_or_null != another.is_positive_or_null) {
    *this += -another;
    return *this;
  }

  if (is_positive_or_null && *this < another) {
    *this = (another - *this);
    invertSign();
    toNormalView();
    return *this;
  }

  if (!is_positive_or_null) {
    if (*this <= another) {
      *this = (-*this += another);
      invertSign();
    } else {
      *this = (-another += *this);
    }
    toNormalView();
  }
  return *this;
}
uint32_t BigInteger::getDegree() { return BigInteger::kDegree; }
uint32_t BigInteger::getBase() { return BigInteger::kBase; }

BigInteger GCD(BigInteger first, BigInteger second) {
  if (!first.isPositiveOrNull()) {
    first.invertSign();
  }
  if (!second.isPositiveOrNull()) {
    second.invertSign();
  }

  while (second != 0) {
    first %= second;
    std::swap(first, second);
  }
  return first;
}

class Rational {
 private:
  BigInteger nominator_ = 0;
  BigInteger denominator_ = 1;
  bool is_positive_or_null = true;
  void to_good_view();
 public:
  Rational() {}
  Rational(int num);
  Rational(const BigInteger& big_int);

  bool operator<(const Rational& another) const;
  bool operator>(const Rational& another) const;
  bool operator<=(const Rational& another) const;
  bool operator>=(const Rational& another) const;
  bool operator==(const Rational& another) const;
  bool operator!=(const Rational& another) const;

  Rational& operator+=(const Rational& another);
  Rational& operator-=(const Rational& another);
  Rational& operator*=(const Rational& another);
  Rational& operator/=(const Rational& another);

  void invertSign();

  Rational operator-() const;

  std::string toString() const;
  std::string asDecimal(size_t precision = 0) const;

  explicit operator double() const;
  explicit operator bool() const;
};

Rational::operator bool() const { return nominator_.isNull(); }

Rational::Rational(int num) {
  if (num < 0) {
    is_positive_or_null = false;
    nominator_ = -num;
  } else {
    nominator_ = num;
  }
  denominator_ = 1;
}

Rational::Rational(const BigInteger& big_int) {
  nominator_ = big_int;
  if (!big_int.isPositiveOrNull()) {
    nominator_.invertSign();
    is_positive_or_null = false;
  }
  denominator_ = 1;
}

bool Rational::operator<(const Rational& another) const {
  if (is_positive_or_null != another.is_positive_or_null) {
    return another.is_positive_or_null;
  }

  BigInteger first = nominator_ * another.denominator_;
  BigInteger second = denominator_ * another.nominator_;
  return is_positive_or_null ? (first < second) : (first > second);
}

bool Rational::operator>(const Rational& another) const {
  return another < *this;
}

bool Rational::operator<=(const Rational& another) const {
  return !(*this > another);
}

bool Rational::operator>=(const Rational& another) const {
  return !(*this < another);
}

bool Rational::operator==(const Rational& another) const {
  if (nominator_ == 0) {
    return another.nominator_ == 0;
  }
  return is_positive_or_null == another.is_positive_or_null &&
         nominator_ == another.nominator_ &&
         denominator_ == another.denominator_;
}

bool Rational::operator!=(const Rational& another) const {
  return !(*this == another);
}

Rational& Rational::operator+=(const Rational& another) {
  BigInteger copy = another.nominator_;
  if (!another.is_positive_or_null) {
    copy = -copy;
  }
  copy *= denominator_;
  nominator_ *= another.denominator_;
  denominator_ *= another.denominator_;
  nominator_ += copy;
  if (!nominator_.isPositiveOrNull()) {
    nominator_.invertSign();
    is_positive_or_null = false;
  }
  to_good_view();
  return *this;
}

Rational& Rational::operator-=(const Rational& another) {
  return *this += (-another);
}

Rational& Rational::operator*=(const Rational& another) {
  nominator_ *= another.nominator_;
  denominator_ *= another.denominator_;
  is_positive_or_null = (is_positive_or_null == another.is_positive_or_null);
  to_good_view();
  return *this;
}

Rational& Rational::operator/=(const Rational& another) {
  nominator_ *= another.denominator_;
  denominator_ *= another.nominator_;
  if (is_positive_or_null != another.is_positive_or_null) {
    is_positive_or_null = false;
  }
  to_good_view();
  return *this;
}

Rational operator+(const Rational& one, const Rational& another) {
  Rational copy = one;
  return copy += another;
}

Rational operator-(const Rational& one, const Rational& another) {
  Rational copy = one;
  return copy -= another;
}

Rational operator*(const Rational& one, const Rational& another) {
  Rational copy = one;
  return copy *= another;
}

Rational operator/(const Rational& one, const Rational& another) {
  Rational copy = one;
  return copy /= another;
}

Rational::operator double() const {
  std::string tmp = asDecimal(BigInteger::getDegree());
  return std::stod(tmp);
}

std::ostream& operator<<(std::ostream& stream, const Rational& big_integer) {
  std::string str = big_integer.toString();
  stream << str;
  return stream;
}

void Rational::to_good_view() {
  BigInteger gcd = GCD(nominator_, denominator_);
  nominator_ /= gcd;
  denominator_ /= gcd;
}

void Rational::invertSign() { is_positive_or_null = !is_positive_or_null; }

Rational Rational::operator-() const {
  Rational copy(*this);
  copy.invertSign();
  return copy;
}

std::string Rational::asDecimal(size_t precision) const {
  BigInteger int_part = (nominator_ / denominator_);
  std::string string = (!is_positive_or_null && *this != 0 ? "-" : "") +
                       int_part.toString() + (precision == 0 ? "" : ".");
  BigInteger copy = nominator_ - (int_part * denominator_);
  size_t len = denominator_.toString().size() - copy.toString().size();
  copy = copy.byPowerOfTen(len);
  if (precision >= len) {
    precision -= len;
  } else {
    string += std::string(precision, '0');
    return string;
  }
  if (copy < denominator_) {
    ++len;
  }
  copy = copy.byPowerOfTen(precision);
  copy /= denominator_;
  std::string tmp_copy = copy.toString();
  string += std::string(len - 1, '0') + tmp_copy;
  return string;
}

std::string Rational::toString() const {
  std::string sign;
  if (!is_positive_or_null && *this != 0) {
    sign = "-";
  }
  if (denominator_ == 1) {
    return sign + nominator_.toString();
  }
  return sign + nominator_.toString() + "/" + denominator_.toString();
}
