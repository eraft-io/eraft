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

#pragma once

#include <strings.h>

#include <string>
#include <vector>

#define CRLF "\r\n"
#define DEFAULT_DATABASE_NAME "default"

enum Error {
  Error_nop = -1,
  Error_ok = 0,
  Error_type = 1,
  Error_exist = 2,
  Error_notExists = 3,
  Error_param = 4,
  Error_unknowCmd = 5,
  Error_nan = 6,
  Error_syntax = 7,
};

extern struct ErrorInfo {
  int len;
  const char* errorStr;
} g_errorInfo[];

class UnboundedBuffer;
void ReplyError(Error err, UnboundedBuffer* reply);

std::size_t FormatOK(UnboundedBuffer* reply);
std::size_t FormatBulk(const char* str, std::size_t len,
                       UnboundedBuffer* reply);
std::size_t FormatBulk(const std::string& str, UnboundedBuffer* reply);
std::size_t PreFormatMultiBulk(std::size_t nBulk, UnboundedBuffer* reply);
std::size_t FormatMultiBulk(const std::vector<std::string> vs,
                            UnboundedBuffer* reply);

std::size_t FormatNull(UnboundedBuffer* reply);

struct NocaseComp {
  bool operator()(const std::string& s1, const std::string& s2) const {
    return strcasecmp(s1.c_str(), s2.c_str()) < 0;
  }

  bool operator()(const char* s1, const std::string& s2) const {
    return strcasecmp(s1, s2.c_str()) < 0;
  }

  bool operator()(const std::string& s1, const char* s2) const {
    return strcasecmp(s1.c_str(), s2) < 0;
  }
};

// new more secure password
#define CLIENT_LONG_PASSWORD 1

// found instead of affected rows
#define CLIENT_FOUND_ROWS 2

// get all colum flags
#define CLIENT_LONG_FLAG 4

// open can specify db on connect
#define CLIENT_CONNECT_WITH_DB 8

// don't allow database.table.column
#define CLIENT_NO_SCHEMA 16

// Can use compression protocol
#define CLIENT_COMPRESS 32

// odbc client
#define CLIENT_ODBC 64

// can use load data local
#define CLIENT_LOCAL_FILES 128

// Ingnore spaces before '('
#define CLIENT_IGNORE_SPACE 256

// New 4.1 protocol
#define CLIENT_PROTOCOL_41 512

// This is an interactive client
#define CLIENT_INTERACTIVE 1024

// switch to ssl after handshake
#define CLIENT_SSL 2048

// ingore sigpipes
#define CLIENT_IGNORE_SIGPIPE 4096

// Client knows about transactions
#define CLIENT_TRANSACTIONS 8192

// Old flag for 4.1 protocol
#define CLIENT_RESERVED 16384

// Old flag for 4.1 authentication
#define CLIENT_SECURE_CONNECTION 32768

// Enable/disable multi-stmt support
#define CLIENT_MULTI_STATEMENTS (1UL << 16)

// Enable/disable multi-results
#define CLIENT_MULTI_RESULTS (1UL << 17)

// Multi-results in PS-protocol
#define CLIENT_PS_MULTI_RESULTS (1UL << 18)

// Client supports plugin authentication
#define CLIENT_PLUGIN_AUTH (1UL << 19)

// Client supports connection attributes
#define CLIENT_CONNECT_ATTRS (1UL << 20)

//  Enable authentication response packet to be larger than 255 bytes
#define CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA (1UL << 21)

// Don't close the connection for a connection with expired password
#define CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS (1UL << 22)

// Capable of handling server state change information. Its a hint to the
// server to include the state change information in Ok packet.
#define CLIENT_SESSION_TRACK (1UL << 23)

// Client no longer needs EOF packet
#define CLIENT_DEPRECATE_EOF (1UL << 24)

#define DEFAULT_SERVER_CAPABILITY                                       \
  (CLIENT_LONG_PASSWORD | CLIENT_LONG_FLAG | CLIENT_CONNECT_WITH_DB |   \
   CLIENT_PROTOCOL_41 | CLIENT_TRANSACTIONS | CLIENT_MULTI_STATEMENTS | \
   CLIENT_PLUGIN_AUTH | CLIENT_DEPRECATE_EOF | CLIENT_SECURE_CONNECTION)

// CharacterSetUtf8 is for UTF8. We use this by default.
#define CHARACTER_SET_UTF8 33

// CharacterSetBinary is for binary. Use by integer fields for instance.
#define CHARACTER_SET_BINARY 63

// SERVER_STATUS_AUTOCOMMIT is the default status of auto-commit.
#define SERVER_STATUS_AUTOCOMMIT 0x0002

// Error codes for server-side errors.
// Originally found in include/mysql/mysqld_error.h
// TODO

#define DEFAULT_AUTH_PLUGIN_NAME "mysql_native_password"

// EOF
#define EOF_PACKET 0xfe

// column TYPE
#define TYPE_NULL_TYPE 0
// INT8 specifies a TINYINT TYPE.
// Properties: 1, IsNumber.
#define TYPE_INT8 257
// UINT8 specifies a TINYINT UNSIGNED TYPE.
// Properties: 2, IsNumber, IsUnsigned.
#define TYPE_UINT8 770
// INT16 specifies a SMALLINT TYPE.
// Properties: 3, IsNumber.
#define TYPE_INT16 259
// UINT16 specifies a SMALLINT UNSIGNED TYPE.
// Properties: 4, IsNumber, IsUnsigned.
#define TYPE_UINT16 772
// INT24 specifies a MEDIUMINT TYPE.
// Properties: 5, IsNumber.
#define TYPE_INT24 261
// UINT24 specifies a MEDIUMINT UNSIGNED TYPE.
// Properties: 6, IsNumber, IsUnsigned.
#define TYPE_UINT24 774
// INT32 specifies a INTEGER TYPE.
// Properties: 7, IsNumber.
#define TYPE_INT32 263
// UINT32 specifies a INTEGER UNSIGNED TYPE.
// Properties: 8, IsNumber, IsUnsigned.
#define TYPE_UINT32 776
// INT64 specifies a BIGINT TYPE.
// Properties: 9, IsNumber.
#define TYPE_INT64 265
// UINT64 specifies a BIGINT UNSIGNED TYPE.
// Properties: 10, IsNumber, IsUnsigned.
#define TYPE_UINT64 778
// FLOAT32 specifies a FLOAT TYPE.
// Properties: 11, IsFloat.
#define TYPE_FLOAT32 1035
// FLOAT64 specifies a DOUBLE or REAL TYPE.
// Properties: 12, IsFloat.
#define TYPE_FLOAT64 1036
// TIMESTAMP specifies a TIMESTAMP TYPE.
// Properties: 13, IsQuoted.
#define TYPE_TIMESTAMP 2061
// DATE specifies a DATE TYPE.
// Properties: 14, IsQuoted.
#define TYPE_DATE 2062
// TIME specifies a TIME TYPE.
// Properties: 15, IsQuoted.
#define TYPE_TIME 2063
// DATETIME specifies a DATETIME TYPE.
// Properties: 16, IsQuoted.
#define TYPE_DATETIME 2064
// YEAR specifies a YEAR TYPE.
// Properties: 17, IsNumber, IsUnsigned.
#define TYPE_YEAR 785
// DECIMAL specifies a DECIMAL or NUMERIC TYPE.
// Properties: 18, None.
#define TYPE_DECIMAL 18
// TEXT specifies a TEXT TYPE.
// Properties: 19, IsQuoted, IsText.
#define TYPE_TEXT 6163
// BLOB specifies a BLOB TYPE.
// Properties: 20, IsQuoted, IsBinary.
#define TYPE_BLOB 10260
// VARCHAR specifies a VARCHAR TYPE.
// Properties: 21, IsQuoted, IsText.
#define TYPE_VARCHAR 6165
// VARBINARY specifies a VARBINARY TYPE.
// Properties: 22, IsQuoted, IsBinary.
#define TYPE_VARBINARY 10262
// CHAR specifies a CHAR TYPE.
// Properties: 23, IsQuoted, IsText.
#define TYPE_CHAR 6167
// BINARY specifies a BINARY TYPE.
// Properties: 24, IsQuoted, IsBinary.
#define TYPE_BINARY 10264
// BIT specifies a BIT TYPE.
// Properties: 25, IsQuoted.
#define TYPE_BIT 2073
// ENUM specifies an ENUM TYPE.
// Properties: 26, IsQuoted.
#define TYPE_ENUM 2074
// SET specifies a SET TYPE.
// Properties: 27, IsQuoted.
#define TYPE_SET 2075
// TUPLE specifies a tuple. This cannot
// be returned in a QueryResult, but it can
// be sent as a bind var.
// Properties: 28, None.
#define TYPE_TUPLE 28
// GEOMETRY specifies a GEOMETRY TYPE.
// Properties: 29, IsQuoted.
#define TYPE_GEOMETRY 2077
// JSON specifies a JSON TYPE.
// Properties: 30, IsQuoted.
#define TYPE_JSON 2078
// EXPRESSION specifies a SQL expression.
// This TYPE is for internal use only.
// Properties: 31, None.
#define TYPE_EXPRESSION 31

#define MYSQL_UNSIGNED 32
#define MYSQL_BINARY 128
#define MYSQL_ENUM 256
#define MYSQL_SET 2048

#define OK_PACKET 0x00
#define ERR_PACKET 0xff

#define INIT_PACKET_CNT 1

#define MYSQL_COM_INIT_DB 0x02

#define MYSQL_COM_QUERY 0x03
