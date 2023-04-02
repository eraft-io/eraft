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
class EStatus {

 public:
  EStatus() {}
  ~EStatus() {}

  /**
   * @brief
   *
   * @param msg
   * @return EStatus
   */
  // static EStatus NotFound(const std::string& msg) {
  //   return EStatus(kNotFound, msg);
  // }

 private:
  /**
   * @brief
   *
   */
  enum StatusCode {
    kOk = 0,
    kNotFound = 1,
    kNotSupport = 2,
  };

  /**
   * @brief Construct a new EStatus object
   *
   * @param code
   * @param msg
   */
  // EStatus(StatusCode code, std::string& msg) {}
};

#endif