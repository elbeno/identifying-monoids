#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

template <typename InputIt, typename OutputIt, typename FwdIt, typename F>
inline OutputIt join(InputIt first, InputIt last, OutputIt dest, FwdIt d_first,
                     FwdIt d_last, F &&f) {
  if (first == last)
    return dest;
  decltype(auto) r = f(*first++);
  dest = std::move(std::begin(r), std::end(r), dest);
  while (first != last) {
    dest = std::copy(d_first, d_last, dest);
    r = f(*first++);
    dest = std::move(std::begin(r), std::end(r), dest);
  }
  return dest;
}

template <typename InputIt, typename OutputIt, typename T, typename F>
inline OutputIt
join(InputIt first, InputIt last, OutputIt dest, const T &delimiter, F &&f,
     std::enable_if_t<std::is_assignable<decltype(*dest), T>::value> * =
         nullptr) {
  // if the delimiter can be assigned directly to the output, do that. example:
  // f produces strings, and delimiter is a char.
  return join(first, last, dest, &delimiter, &delimiter + 1,
              std::forward<F>(f));
}

template <typename InputIt, typename OutputIt, typename T, typename F>
inline OutputIt
join(InputIt first, InputIt last, OutputIt dest, const T &delimiter, F &&f,
     std::enable_if_t<!std::is_assignable<decltype(*dest), T>::value> * =
         nullptr) {
  // if the delimiter can't be assigned to the output, we assume it's a range of
  // values that can be. example: f produces strings, and delimiter is a string.
  return join(first, last, dest, std::begin(delimiter), std::end(delimiter),
              std::forward<F>(f));
}

template <typename InputIt, typename OutputIt, typename F, size_t N>
inline OutputIt join(InputIt first, InputIt last, OutputIt dest,
                     const char (&delimiter)[N], F &&f) {
  // for const char array literals, they are null-terminated: we don't want the
  // null character.
  return join(first, last, dest, &delimiter[0], &delimiter[N - 1],
              std::forward<F>(f));
}

template <typename InputIt, typename OutputIt, typename T>
inline OutputIt join(InputIt first, InputIt last, OutputIt dest,
                     const T &delimiter) {
  // if a function is not supplied, assume the identity function
  return join(first, last, dest, delimiter,
              [](auto &&x) { return std::forward<decltype(x)>(x); });
}

#include <iostream>
#include <map>
#include <string>
#include <vector>

int main(void) {
  {
    std::vector<int> v = {1, 2, 3, 4, 5};
    std::string s;
    auto it = join(std::cbegin(v), std::cend(v), std::back_inserter(s), ",",
                   [](int i) { return std::to_string(i); });
    std::cout << s << std::endl;
  }
  {
    std::vector<std::string> v = {"foo", "bar", "baz", "quux"};
    std::string s;
    auto it =
        join(std::make_move_iterator(std::begin(v)),
             std::make_move_iterator(std::end(v)), std::back_inserter(s), ",");
    std::cout << s << std::endl;
  }
  {
    std::vector<std::string> v = {"foo", "bar", "baz", "quux"};
    std::string s;
    auto it = join(std::begin(v), std::end(v), std::back_inserter(s), ",");
    std::cout << s << std::endl;
  }

  {
    std::string url_base = "https://example.com/?";
    std::map<std::string, std::string> url_args{{"alpha", "able"},
                                                {"bravo", "baker"}};

    join(std::cbegin(url_args), std::cend(url_args),
         std::back_inserter(url_base), '&', [](const auto &p) {
           const auto &[key, val] = p;
           return key + '=' + val;
         });
    std::cout << url_base << std::endl;
  }
}
