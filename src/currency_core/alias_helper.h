// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "currency_core/currency_basic.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet/core_rpc_proxy.h"
//---------------------------------------------------------------
namespace tools
{
  template<typename callback_t>
  bool get_transfer_address_cb(const std::string& adr_str, currency::account_public_address& addr, currency::payment_id_t& payment_id, callback_t cb)
  {
    if (!adr_str.size())
      return false;

    std::string addr_str_local = adr_str;

    if (adr_str[0] == '@')
    {
      //referred by alias name
      if (adr_str.size() < 2)
        return false;
      std::string pure_alias_name = adr_str.substr(1);
      CHECK_AND_ASSERT_MES(currency::validate_alias_name(pure_alias_name), false, "wrong name set in transfer command");


      currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req_alias_info = AUTO_VAL_INIT(req_alias_info);
      req_alias_info.alias = pure_alias_name;
      currency::COMMAND_RPC_GET_ALIAS_DETAILS::response alias_info = AUTO_VAL_INIT(alias_info);

      if (!cb(req_alias_info, alias_info))
        return false;

      if (alias_info.status != CORE_RPC_STATUS_OK || !alias_info.alias_details.address.size())
        return false;

      addr_str_local = alias_info.alias_details.address;
    }

    if (!get_account_address_and_payment_id_from_str(addr, payment_id, addr_str_local))
    {
      return false;
    }
    return true;
  }

  inline 
  bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, currency::payment_id_t& payment_id, i_core_proxy* proxy)
  {
    return get_transfer_address_cb(adr_str, addr, payment_id, [&proxy](currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req_alias_info,
                                                                        currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& alias_info)
    {
      return proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req_alias_info, alias_info);
    });
  }

  template<typename core_rpc_server_t>
  bool get_transfer_address_t(const std::string& adr_str, currency::account_public_address& addr, currency::payment_id_t& payment_id, core_rpc_server_t& srv)
  {
    return get_transfer_address_cb(adr_str, addr, payment_id, [&srv](currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req_alias_info,
      currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& alias_info)
    {
      epee::json_rpc::error stub;
      epee::net_utils::connection_context_base stub2;
      return srv.on_get_alias_details(req_alias_info, alias_info, stub, stub2);
    });
  }

}
