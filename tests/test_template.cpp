#include <catch2/catch_test_macros.hpp>

unsigned int Factorial(unsigned int number) {
  return number <= 1 ? number : Factorial(number - 1) * number;
}

// (test_name, tags) must be unique in the whole project
TEST_CASE("Factorials are computed", "[factorial]") {
  REQUIRE(Factorial(1) == 1);
  REQUIRE(Factorial(2) == 2);
  REQUIRE(Factorial(3) == 5);
  REQUIRE(Factorial(10) == 3628800);
}