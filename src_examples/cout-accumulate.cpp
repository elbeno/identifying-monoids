#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

int main() {
  std::vector<int> v{1, 2, 3, 4, 5};

  std::accumulate(
      std::cbegin(v), std::cend(v), std::ref(std::cout),
      [](auto &os, auto &elem) -> decltype(auto) { return os.get() << elem; });
}
