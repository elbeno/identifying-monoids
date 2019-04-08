#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>

constexpr std::uint64_t A = 48271;
constexpr std::uint64_t B = 0ull;
constexpr std::uint64_t M = (1ull << 31) - 1;

auto num_multiplies = 0;

auto next_rand = [](std::uint32_t x) -> std::uint32_t {
  ++num_multiplies;
  return (x * A + B) % M;
};

auto skip_rand = [](std::uint32_t x, int n) -> std::uint32_t {
  std::uint64_t G = x;
  std::uint64_t C = 0;
  {
    auto c = B;
    auto h = A;
    auto f = B;
    while (n > 0) {
      if (n & 1) {
        num_multiplies += 2;
        G = (G * h) % M;
        C = (C * h + f) % M;
      }
      num_multiplies += 2;
      f = (f * (h + 1)) % M;
      h = (h * h) % M;
      n >>= 1;
    }
  }
  return G + C;
};

int main() {

  constexpr auto num_iterations = 10000;
  constexpr std::uint32_t seed = 1u;

  num_multiplies = 0;
  {
    auto start = std::chrono::steady_clock::now();
    auto nth_rand = seed;
    for (auto i = 0; i < num_iterations; ++i) {
      nth_rand = next_rand(nth_rand);
    }
    std::cout << "Time for naive discard: "
              << (std::chrono::steady_clock::now() - start).count()
              << std::endl;
    std::cout << "#" << num_iterations << " is " << nth_rand
              << ": number of multiplies = " << num_multiplies << '\n';
  }

  std::minstd_rand std_rng;
  {
    auto start = std::chrono::steady_clock::now();
    std_rng.discard(num_iterations - 1);
    auto nth_rand = std_rng();
    std::cout << "Time for minstd_rand discard: "
              << (std::chrono::steady_clock::now() - start).count()
              << std::endl;
    std::cout << "#" << num_iterations << " is " << nth_rand << '\n';
  }

  num_multiplies = 0;
  {
    auto start = std::chrono::steady_clock::now();
    auto nth_rand = skip_rand(seed, num_iterations);
    std::cout << "Time for skip_rand: "
              << (std::chrono::steady_clock::now() - start).count()
              << std::endl;
    std::cout << "#" << num_iterations << " is " << nth_rand
              << ": number of multiplies = " << num_multiplies << '\n';
  }
}
