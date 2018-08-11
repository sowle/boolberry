// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "command_line.h"
#include "currency_config.h"

namespace command_line
{
  const arg_descriptor<bool>        arg_help =         { "help", "Produce help message"};
  const arg_descriptor<bool>        arg_version =      { "version", "Output version information"};
  const arg_descriptor<std::string> arg_data_dir =     { "data-dir", "Specify data directory", };

  const arg_descriptor<std::string> arg_config_file =  { "config-file", "Specify configuration file", std::string(CURRENCY_NAME_SHORT ".conf") };
  const arg_descriptor<bool>        arg_os_version =   { "os-version", "" };
  const arg_descriptor<std::string> arg_log_file =     { "log-file", "", "" };
  const arg_descriptor<int>         arg_log_level =    { "log-level", "", LOG_LEVEL_0 };
  const arg_descriptor<bool>        arg_console =      { "no-console", "Disable daemon console commands" };
  const arg_descriptor<bool>        arg_show_details = { "currency-details", "Display currency details" };

}
