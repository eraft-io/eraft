/**
 * @file command_handler.h
 * @author jay_jieliu@outlook.com
 * @brief
 * @version 0.1
 * @date 2023-06-24
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <vector>

#include "client.h"
#include "estatus.h"

class Client;

class CommandHandler {
 public:
  virtual EStatus Execute(const std::vector<std::string>& params,
                          Client*                         cli) = 0;
  virtual ~CommandHandler() {}
};

class InfoCommandHandler : public CommandHandler {
 public:
  EStatus Execute(const std::vector<std::string>& params, Client* cli);

  InfoCommandHandler();
  ~InfoCommandHandler();
};

class SetCommandHandler : public CommandHandler {
 public:
  EStatus Execute(const std::vector<std::string>& params, Client* cli);

  SetCommandHandler();
  ~SetCommandHandler();
};

class GetCommandHandler : public CommandHandler {
 public:
  EStatus Execute(const std::vector<std::string>& params, Client* cli);

  GetCommandHandler();
  ~GetCommandHandler();
};

class DelCommandHandler : public CommandHandler {
 public:
  EStatus Execute(const std::vector<std::string>& params, Client* cli);

  DelCommandHandler();
  ~DelCommandHandler();
};

class UnKnowCommandHandler : public CommandHandler {
 public:
  EStatus Execute(const std::vector<std::string>& params, Client* cli);
  UnKnowCommandHandler();
  ~UnKnowCommandHandler();
};

class ShardGroupCommandHandler : public CommandHandler {
 public:
  EStatus Execute(const std::vector<std::string>& params, Client* cli);
  ShardGroupCommandHandler();
  ~ShardGroupCommandHandler();
};

#endif
