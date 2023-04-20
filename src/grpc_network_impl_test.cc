#include <gtest/gtest.h>

#include <iostream>

#include "eraftkv.pb.h"

// TEST(GrpcTest, TestInit) {
//   ASSERT_EQ(1, 1);
// }

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}