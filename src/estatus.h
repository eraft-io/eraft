/**
 * @file estatus.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef ESTATUS_H_
#define ESTATUS_H_
#include <string>

/**
 * @brief
 *
 */
enum class EStatus {
  kOk = 0,
  kNotFound = 1,
  kNotSupport = 2,
  kPutKeyToRocksDBErr = 3,
};

#endif