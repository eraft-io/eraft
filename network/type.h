// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// !!! WARRING !!!: support mysql protocol version <= mysql-5.1.70
//

#pragma once
#include <unistd.h>

#include <map>

#include "common.h"

namespace Protocol {

std::map<int64_t, int32_t> MYSQL_TO_TYPE_MAP = {
    {1, TYPE_INT8},      {2, TYPE_INT16},     {3, TYPE_INT32},
    {4, TYPE_FLOAT32},   {5, TYPE_FLOAT64},   {6, TYPE_NULL_TYPE},
    {7, TYPE_TIMESTAMP}, {8, TYPE_INT64},     {9, TYPE_INT24},
    {10, TYPE_DATE},     {11, TYPE_TIME},     {12, TYPE_DATETIME},
    {13, TYPE_YEAR},     {16, TYPE_BIT},      {245, TYPE_JSON},
    {246, TYPE_DECIMAL}, {249, TYPE_TEXT},    {250, TYPE_TEXT},
    {251, TYPE_TEXT},    {252, TYPE_TEXT},    {253, TYPE_VARCHAR},
    {254, TYPE_CHAR},    {255, TYPE_GEOMETRY}};

std::map<int32_t, std::pair<int64_t, int64_t> > TYPE_TO_MYSQL = {
    {TYPE_INT8, {1, 0}},
    {TYPE_UINT8, {1, MYSQL_UNSIGNED}},
    {TYPE_INT16, {2, 0}},
    // Uint16:    {typ: 2, flags: mysqlUnsigned},
    {TYPE_UINT16, {2, MYSQL_UNSIGNED}},
    // Int32:     {typ: 3},
    {TYPE_INT32, {3, 0}},
    // Uint32:    {typ: 3, flags: mysqlUnsigned},
    {TYPE_UINT32, {3, MYSQL_UNSIGNED}},
    // Float32:   {typ: 4},
    {TYPE_FLOAT32, {4, 0}},
    // Float64:   {typ: 5},
    {TYPE_FLOAT64, {5, 0}},
    // Null:      {typ: 6, flags: mysqlBinary},
    {TYPE_NULL_TYPE, {6, MYSQL_BINARY}},
    // Timestamp: {typ: 7},
    {TYPE_TIMESTAMP, {7, 0}},
    // Int64:     {typ: 8},
    {TYPE_INT64, {8, 0}},
    // Uint64:    {typ: 8, flags: mysqlUnsigned},
    {TYPE_UINT64, {8, MYSQL_UNSIGNED}},
    // Int24:     {typ: 9},
    {TYPE_INT24, {9, 0}},
    // Uint24:    {typ: 9, flags: mysqlUnsigned},
    {TYPE_UINT24, {9, MYSQL_UNSIGNED}},
    // Date:      {typ: 10, flags: mysqlBinary},
    {TYPE_DATE, {10, MYSQL_BINARY}},
    // Time:      {typ: 11, flags: mysqlBinary},
    {TYPE_TIME, {11, MYSQL_BINARY}},
    // Datetime:  {typ: 12, flags: mysqlBinary},
    {TYPE_DATETIME, {12, MYSQL_BINARY}},
    // Year:      {typ: 13, flags: mysqlUnsigned},
    {TYPE_YEAR, {13, MYSQL_UNSIGNED}},
    // Bit:       {typ: 16, flags: mysqlUnsigned},
    {TYPE_BIT, {16, MYSQL_UNSIGNED}},
    // TypeJSON:  {typ: 245},
    {TYPE_JSON, {245, 0}},
    // Decimal:   {typ: 246},
    {TYPE_DECIMAL, {246, 0}},
    // Text:      {typ: 252},
    {TYPE_TEXT, {252, 0}},
    // Blob:      {typ: 252, flags: mysqlBinary},
    {TYPE_BLOB, {252, MYSQL_BINARY}},
    // VarChar:   {typ: 253},
    {TYPE_VARCHAR, {253, 0}},
    // VarBinary: {typ: 253, flags: mysqlBinary},
    {TYPE_VARBINARY, {253, MYSQL_BINARY}},
    // Char:      {typ: 254},
    {TYPE_CHAR, {254, 0}},
    // Binary:    {typ: 254, flags: mysqlBinary},
    {TYPE_BINARY, {254, MYSQL_BINARY}},
    // Enum:      {typ: 254, flags: mysqlEnum},
    {TYPE_ENUM, {254, MYSQL_ENUM}},
    // Set:       {typ: 254, flags: mysqlSet},
    {TYPE_SET, {254, MYSQL_SET}},
    // Geometry:  {typ: 255},
    {TYPE_GEOMETRY, {255, 0}}};

int32_t MySQLToType(int64_t mysqlType, int64_t flags) {
  int32_t typ = MYSQL_TO_TYPE_MAP[mysqlType];
  switch (typ) {
    case TYPE_INT8:
      if ((flags & MYSQL_UNSIGNED) != 0) {
        return TYPE_UINT8;
      }
      return TYPE_INT8;
    case TYPE_INT16:
      if ((flags & MYSQL_UNSIGNED) != 0) {
        return TYPE_UINT16;
      }
      return TYPE_INT16;
    case TYPE_INT32:
      if ((flags & MYSQL_UNSIGNED) != 0) {
        return TYPE_UINT32;
      }
      return TYPE_INT32;
    case TYPE_INT64:
      if ((flags & MYSQL_UNSIGNED) != 0) {
        return TYPE_UINT64;
      }
      return TYPE_INT64;
    case TYPE_INT24:
      if ((flags & MYSQL_UNSIGNED) != 0) {
        return TYPE_UINT24;
      }
      return TYPE_INT24;
    case TYPE_TEXT:
      if ((flags & MYSQL_BINARY) != 0) {
        return TYPE_BLOB;
      }
      return TYPE_TEXT;
    case TYPE_VARCHAR:
      if ((flags & MYSQL_BINARY) != 0) {
        return TYPE_VARBINARY;
      }
      return TYPE_VARCHAR;
    case TYPE_CHAR:
      if ((flags & MYSQL_BINARY) != 0) {
        return TYPE_BINARY;
      }
      if ((flags & MYSQL_ENUM) != 0) {
        return TYPE_ENUM;
      }
      if ((flags & MYSQL_SET) != 0) {
        return TYPE_SET;
      }
      return TYPE_CHAR;
  }
  return typ;
}

}  // namespace Protocol