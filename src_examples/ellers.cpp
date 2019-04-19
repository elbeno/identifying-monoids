#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <random>
#include <type_traits>

static int next_set_id = 0;
struct cell {
  int set_id{};
  bool connected_east{};
  bool connected_south{};
};

auto merge_cells = [](auto old_id, auto new_id, auto first, auto last) {
  std::for_each(first, last, [new_id = new_id, old_id = old_id](auto &cell) {
    if (cell.set_id == old_id)
      cell.set_id = new_id;
  });
};

auto print_row_north_edges = [](std::size_t cols) {
  std::cout << '+';
  for (; cols > 0; --cols) {
    std::cout << "--+";
  }
  std::cout << '\n';
};

auto print_row_east_edges = [](const auto &row) {
  std::cout << '|';
  for (const auto &cell : row) {
    if (not cell.connected_east) {
      std::cout << "  |";
    } else {
      std::cout << "   ";
    }
  }
  std::cout << '\n';
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

auto next_row = [](auto &row) {
  std::remove_reference_t<decltype(row)> new_row{};
  for (auto i = 0u; i < std::size(row); ++i) {
    if (row[i].connected_south) {
      new_row[i].set_id = row[i].set_id;
    } else {
      new_row[i].set_id = next_set_id++;
    }
  }
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

int main() {
  constexpr std::size_t num_cols = 15;
  constexpr std::size_t num_rows = 15;

  std::array<cell, num_cols> row{};
  print_row_north_edges(std::size(row));

  for (int i = 0; i < num_rows - 1; ++i) {
    auto new_row = next_row(row);
    row.swap(new_row);
    randomly_carve_east(row);
    randomly_carve_south(row);
    print_row_east_edges(row);
    print_row_south_edges(row);
  }
  auto new_row = next_row(row);
  row.swap(new_row);
  last_carve_east(row);
  print_row_east_edges(row);
  print_row_north_edges(std::size(row));
}
