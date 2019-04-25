#include <array>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>

template <typename ForwardIt, typename T, typename UnaryFunction>
void iota(ForwardIt first, ForwardIt last, T value, UnaryFunction f) {
  std::accumulate(first, last, value, [&](auto &so_far, auto &next) {
    next = so_far;
    return f(so_far);
  });
}

template <typename ForwardIt, typename T>
void iota(ForwardIt first, ForwardIt last, T value) {
  std::accumulate(first, last, value, [](const auto &so_far, auto &next) {
    next = so_far;
    return so_far + 1;
  });
}

template <typename ForwardIt>
decltype(auto) fold_print(ForwardIt first, ForwardIt last) {
  return std::accumulate(first, last, std::ref(std::cout),
                         [](auto &os, auto &elem) -> decltype(auto) {
                           return os.get() << elem << ' ';
                         })
      .get();
}

int main(int argc, char *argv[]) {
  std::array<int, 5> a;
  iota(std::begin(a), std::end(a), 2);
  fold_print(std::cbegin(a), std::cend(a)) << '\n';

  std::array<int, 5> b;
  iota(std::begin(b), std::end(b), 2, [](auto x) { return x * 2; });
  fold_print(std::cbegin(b), std::cend(b)) << '\n';

  return 0;
}
