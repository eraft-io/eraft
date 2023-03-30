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

#include <string>

/**
 * @brief
 *
 */
class EStatus {

 public:
  EStatus();
  ~EStatus();

  /**
   * @brief
   *
   * @param msg
   * @return EStatus
   */
  static EStatus NotFound(const std::string& msg) {
    return Status(kNotFound, msg);
  }

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
  EStatus(StatusCode code, std::string& msg) {}
};
