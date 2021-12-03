#pragma once

/**
 *  @file CppStringPlus.hpp
 *
 *  这个头文件定义了一些对标准 c++ string 扩展函数
 *
 *  Copyright © 2014-2019 by LiuJ
 */

#include <stdint.h>

#include <string>

namespace CppStringPlus {

std::string vsprintf(const char* format, va_list args);
/**
 *  类似于 c 标准库里面的 sprintf函数
 */
std::string sprintf(const char* format, ...);

/**
 * 这个函数读入一个 string 对象，将其中所有大学字母转成小写字母，返回结果
 *
 * @param[in] inString
 *   将要被转换的 string
 *
 * @return
 *   所有大写字母被转小写后的字符串
 */
std::string ToLower(const std::string& inString);

/**
 * 转 string 到整数过程中可能出现结果枚举
 */
enum class ToIntegerResult {
  Success,

  NotANumber,

  Overflow
};

/**
 * 这个函数将给定的 string
 * 对象转换为整数，如果输入字符串不合法，或者溢出，转换将失败
 * @param[in] numberString
 *      待转换包含数字的字符串
 *
 * @param[out] number
 *      存储转后后结果
 *
 * @return
 *
 */
ToIntegerResult ToInteger(const std::string& numberString, intmax_t& number);

}  // namespace CppStringPlus
