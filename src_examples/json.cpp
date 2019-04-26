#include <algorithm>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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

struct json_wrapper_t;
using json_array_t = std::vector<json_wrapper_t>;
using json_object_t = std::map<std::string, json_wrapper_t>;
using json_value_t = std::variant<bool, double, std::string, std::nullptr_t,
                                  json_array_t, json_object_t>;

struct json_wrapper_t {
  json_value_t value;
  operator json_value_t &() { return value; }
  operator const json_value_t &() const { return value; }
};

template <typename... Ts, typename T, typename F, auto... Is>
decltype(auto) fold(const std::variant<Ts...> &v, T init, F f,
                    std::index_sequence<Is...>) {
  // The result type is the same for each function, so just take the first.
  using result_type = decltype(f(init, std::get<0>(v)));

  // The "handler_type" will be a lambda that doesn't capture, so decays to a
  // function pointer. We will wrap the index in a type (integral_constant)
  // and unwrap its value inside the lambda to access the right fields.
  using handler_type = result_type (*)(const std::variant<Ts...> &, T, F);
  auto make_handler = [](auto int_const) {
    return [](const std::variant<Ts...> &v, T init, F f) -> result_type {
      constexpr auto I = decltype(int_const)::value;
      return f(init, std::get<I>(v));
    };
  };

  // Now all we need is to expand the index_sequence pack to create handlers,
  // and call the correct one for the variant.
  static handler_type handlers[] = {
      make_handler(std::integral_constant<size_t, Is>{})...};
  return handlers[v.index()](v, init, f);
}

template <typename... Ts, typename T, typename F>
decltype(auto) fold(const std::variant<Ts...> &v, T init, F f) {
  return fold(v, init, f, std::index_sequence_for<Ts...>{});
}

template <typename T> const auto render = nullptr;

template <>
const auto render<json_value_t> =
    [](auto &&os, const json_value_t &jsv) -> decltype(auto) {
  return fold(jsv, os, [](auto &os, auto &&value) -> decltype(auto) {
    using type = std::decay_t<decltype(value)>;
    return render<type>(os, std::forward<decltype(value)>(value));
  });
};

template <>
const auto render<bool> = [](auto &&os, bool b) -> decltype(auto) {
  return os.get() << (b ? "true" : "false");
};

template <>
const auto render<double> = [](auto &&os, double d) -> decltype(auto) {
  return os.get() << std::to_string(d);
};

template <>
const auto render<std::string> =
    [](auto &&os, const std::string &s) -> decltype(auto) {
  return os.get() << std::quoted(s);
};

template <>
const auto render<std::nullptr_t> =
    [](auto &&os, std::nullptr_t) -> decltype(auto) {
  return os.get() << "null";
};

template <>
const auto render<json_array_t> =
    [](auto &&os, const json_array_t &a) -> decltype(auto) {
  return std::accumulate(std::cbegin(a), std::cend(a),
                         std::ref(os.get() << '['),
                         [](auto &os, auto &elem) -> decltype(auto) {
                           return render<json_value_t>(os, elem) << ',';
                         })
             .get()
         << ']';
};

template <>
const auto render<json_object_t> =
    [](auto &&os, const json_object_t &o) -> decltype(auto) {
  return std::accumulate(std::cbegin(o), std::cend(o),
                         std::ref(os.get() << '{'),
                         [](auto &os, auto &elem) -> decltype(auto) {
                           render<std::string>(os, elem.first) << ':';
                           return render<json_value_t>(os, elem.second) << ',';
                         })
             .get()
         << '}';
};

int main(int argc, char *argv[]) {
  json_object_t jso{
      {"number", {{42.0}}},
      {"string", {{std::string{"Hello, \"world\""}}}},
      {"bool", {{true}}},
      {"array", {json_array_t{{1.0}, {2.0}, {3.0}, {4.0}}}},
      {"null", {nullptr}},
      {"object", {json_object_t{{"foo", {{true}}}, {"bar", {{false}}}}}}};

  render<json_object_t>(std::ref(std::cout), jso) << '\n';
  return 0;
}
