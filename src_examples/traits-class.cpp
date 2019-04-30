#include <type_traits>

template <typename T> concept Integral = std::is_integral<T>::value;

template <typename T, typename Name> struct monoid_traits;

template <Integral T> struct monoid_traits<T, class multiply> {
  constexpr static auto identity = [] { return T{1}; };
  constexpr static auto op = [](T a, T b) { return a * b; };
};

template <Integral T> struct monoid_traits<T, class add> {
  constexpr static auto identity = [] { return T{0}; };
  constexpr static auto op = [](T a, T b) { return a + b; };
};

template <typename Name, typename... Ts> constexpr auto fold(Ts... ts) {
  using T = std::common_type_t<Ts...>;
  using monoid = monoid_traits<T, Name>;
  T sum = monoid::identity();
  return ((sum = monoid::op(sum, ts)), ...);
};

int main() {
  auto x = 2;
  auto y = 3;
  // return fold<class multiply>(x, y);
  return fold<class add>(x, y);
}
