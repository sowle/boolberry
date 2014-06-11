// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <boost/program_options.hpp>
#include "include_base_utils.h"
#include "version.h"

using namespace epee;

#include "console_handler.h"
#include "p2p/net_node.h"
#include "currency_core/checkpoints_create.h"
#include "currency_core/currency_core.h"
#include "rpc/core_rpc_server.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "daemon/daemon_commands_handler.h"
//#include "common/miniupnp_helper.h"
#include "view_iface.h"
#include "daemon_rpc_proxy.h"
#include "wallet/wallet2.h"

namespace po = boost::program_options;

#if defined(WIN32)
#include <crtdbg.h>
#endif

//TODO: need refactoring here. (template classes can't be used in BOOST_CLASS_VERSION)
BOOST_CLASS_VERSION(nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >, CURRENT_P2P_STORAGE_ARCHIVE_VER);

class daemon_backend: public tools::i_wallet2_callback
{
public:
  daemon_backend();
  ~daemon_backend();
  bool start(int argc, char* argv[], view::i_view* pview_handler);
  bool stop();
  bool send_stop_signal();
  bool open_wallet(const std::string& path, const std::string& password);
private:
  void main_worker(const po::variables_map& vm);
  bool update_state_info();
  bool update_wallets();
  void loop();

  //----- tools::i_wallet2_callback ------
  virtual void on_new_block(uint64_t height, const currency::block& block);
  virtual void on_money_received(uint64_t height, const currency::transaction& tx, size_t out_index);
  virtual void on_money_spent(uint64_t height, const currency::transaction& in_tx, size_t out_index, const currency::transaction& spend_tx);

  std::thread m_main_worker_thread;
  std::atomic<bool> m_stop_singal_sent;
  view::view_stub m_view_stub;
  view::i_view* m_pview;
  tools::daemon_rpc_proxy_fast m_rpc_proxy;
  critical_section m_wallet_lock;
  tools::wallet2 m_wallet;
  std::atomic<uint64_t> m_last_daemon_height;
  std::atomic<uint64_t> m_last_wallet_synch_height;

  //daemon stuff
  currency::core m_ccore;
  currency::t_currency_protocol_handler<currency::core> m_cprotocol;
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > m_p2psrv;
  currency::core_rpc_server m_rpc_server;
};




