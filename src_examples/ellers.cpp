#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

namespace nonstd {
template <class ForwardIt, class T, class EndoFunction>
constexpr auto iterate(ForwardIt first, ForwardIt last, T init,
                       EndoFunction f) {
  while (first != last) {
    *first++ = init;
    init = f(std::move(init));
  }
  return init;
}

template <class ForwardIt, class T, class Size, class EndoFunction>
constexpr auto iterate_n(ForwardIt first, Size n, T &&init, EndoFunction f) {
  while (n--) {
    *first++ = init;
    init = f(std::move(init));
  }
  return init;
}

template <class InputIt, class Size, class T, class BinaryOp>
constexpr auto accumulate_n(InputIt first, Size n, T init, BinaryOp op)
    -> std::pair<T, InputIt> {
  for (; n > 0; --n, ++first) {
    init = op(std::move(init), *first);
  }
  return {init, first};
}
} // namespace nonstd

template <typename ForwardIt, typename T, typename Proj>
decltype(auto) fold_print(ForwardIt first, ForwardIt last, T init, Proj f) {
  return std::accumulate(first, last, std::ref(std::cout << init),
                         [&](auto &os, auto &elem) -> decltype(auto) {
                           return os.get() << f(elem);
                         })
      .get();
}

static int next_set_id = 0;
struct cell {
  int set_id{next_set_id++};
  bool connected_east{};
  bool connected_south{};
};

auto merge_cells = [](auto old_id, auto new_id, auto first, auto last) {
  std::for_each(first, last, [new_id = new_id, old_id = old_id](auto &cell) {
    if (cell.set_id == old_id)
      cell.set_id = new_id;
  });
};

auto print_row_north_edges = [](const auto &row) {
  fold_print(std::cbegin(row), std::cend(row), '+', [](auto) { return "--+"; })
      << '\n';
};

auto print_row_east_edges = [](const auto &row) {
  fold_print(std::cbegin(row), std::cend(row), '|', [](auto &cell) {
    return cell.connected_east ? "   " : "  |";
  }) << '\n';
};

auto print_row_south_edges = [](const auto &row) {
  std::cout << '+';
  for (auto i = 0u; i < std::size(row) - 1; ++i) {
    if (row[i].connected_south and row[i + 1].connected_south and
        row[i].connected_east) {
      std::cout << "   ";
    } else if (not row[i].connected_south) {
      std::cout << "--+";
    } else {
      std::cout << "  +";
    }
  }

  const auto i = std::size(row) - 1;
  if (not row[i].connected_south) {
    std::cout << "--+";
  } else {
    std::cout << "  +";
  }
  std::cout << '\n';
};

std::minstd_rand rng{std::random_device{}()};

auto randomly_carve_east = [](auto &row) {
  std::uniform_int_distribution<int> dis(0, 1);
  for (auto i = 0u; i < std::size(row) - 1; ++i) {
    if (dis(rng) and row[i].set_id != row[i + 1].set_id) {
      row[i].connected_east = true;
      merge_cells(row[i + 1].set_id, row[i].set_id, std::begin(row),
                  std::end(row));
      ++i;
    }
  }
};

auto randomly_carve_south = [](auto &row) {
  auto first = std::begin(row);
  while (first != std::end(row)) {
    // get the cells in this set
    auto last = std::find_if(
        first, std::end(row),
        [id = first->set_id](const auto &cell) { return cell.set_id != id; });

    // carve some
    auto n = std::distance(first, last);
    std::uniform_int_distribution<int> dis(0, 1);
    auto num_connected = 0;
    for (int i = 0; i < n; ++i) {
      if (dis(rng)) {
        ++num_connected;
        std::next(first, i)->connected_south = true;
      }
    }
    // carve at least one
    if (num_connected == 0) {
      std::uniform_int_distribution<int> dis(0, n - 1);
      std::next(first, dis(rng))->connected_south = true;
    }

    first = last;
  }
};

auto next_row = [](auto &&row) {
  auto new_row = std::forward<decltype(row)>(row);
  std::for_each(std::begin(new_row), std::end(new_row), [](auto &cell) {
    cell.connected_east = false;
    if (not std::exchange(cell.connected_south, false)) {
      cell.set_id = next_set_id++;
    }
  });
  return new_row;
};

auto last_carve_east = [](auto &row) {
  for (auto i = 0u; i < std::size(row) - 1; ++i) {
    if (row[i].set_id != row[i + 1].set_id) {
      row[i].connected_east = true;
      merge_cells(row[i + 1].set_id, row[i].set_id, std::begin(row),
                  std::end(row));
    }
  }
};

struct printer {
  printer &operator*() { return *this; }
  printer &operator++() { return *this; }
  printer &operator++(int) { return *this; }

  template <typename T> printer &operator=(const T &t) {
    print_row_east_edges(t);
    print_row_south_edges(t);
    return *this;
  };
};

int main(int argc, char *argv[]) {
  std::size_t num_rows = std::atoi(argv[1]);
  std::size_t num_cols = std::atoi(argv[2]);

  std::vector<cell> row{num_cols};
  print_row_north_edges(row);
  randomly_carve_east(row);
  randomly_carve_south(row);

  row = nonstd::iterate_n(
      printer{}, num_rows - 1, std::move(row), [&](auto &&current) {
        auto next = next_row(std::forward<decltype(current)>(current));
        randomly_carve_east(next);
        randomly_carve_south(next);
        return next;
      });

  last_carve_east(row);
  print_row_east_edges(row);
  print_row_north_edges(row);
}
