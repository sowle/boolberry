#pragma once 

#include <boost/atomic.hpp>
#include <boost/program_options.hpp>
#include <atomic>
#include "currency_basic.h"
#include "difficulty.h"
#include "common/command_line.h"
#include "net/levin_server_cp2.h"
#include "storages/levin_abstract_invoke2.h"
#include "pool_commands_defs.h"
#include "math_helper.h"
#include "miner.h"

#define POOL_DEFAULT_INVOKE_TIMEOUT                20000      //20 seconds

namespace currency
{

  namespace
  {
    const command_line::arg_descriptor<std::string>         arg_pool_credentials_file   = {"pool-credentials-file", "Setup credentials file here for builtin pool", "", false};
    const command_line::arg_descriptor<uint32_t>            arg_pool_listen_port   = {"pool-bind-port", "Setup listening port for builtin pool", 0, false};
  }
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct pool_connection_context: public epee::net_utils::connection_context_base
  {
    std::string user_login;
    tools::height_info hi;
  };

  struct pool_credential
  {
    std::string login;
    std::string pass;
    std::string address;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(login)
      KV_SERIALIZE(pass)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  struct pool_credentials
  {
    std::list<pool_credential> credentials;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(login)
    END_KV_SERIALIZE_MAP()
  };

  class poolminer: public epee::levin::levin_commands_handler<pool_connection_context>
  {
  public: 
    poolminer(i_miner_handler* phandler, blockchain_storage& bc);
    ~poolminer();
    bool init(const boost::program_options::variables_map& vm);
    static void init_options(boost::program_options::options_description& desc_cmd_only);
    bool set_block_template(const block& bl, const difficulty_type& diffic, uint64_t height);
    bool on_block_chain_update();
    bool on_idle();
    void send_stop_signal();
    bool start();
    bool stop();
    bool is_running();
    bool request_block_template();

  private:
    bool worker_thread();
    typedef epee::net_utils::boosted_tcp_server<epee::levin::async_protocol_handler<pool_connection_context> > net_server;


    CHAIN_LEVIN_INVOKE_MAP2(pool_connection_context); 
    CHAIN_LEVIN_NOTIFY_MAP2(pool_connection_context); 

    BEGIN_INVOKE_MAP2(poolminer)
      HANDLE_NOTIFY_T2(tools::COMMAND_RPC_LOGIN, &poolminer::handle_login)
      HANDLE_NOTIFY_T2(tools::COMMAND_RPC_GETJOB, &poolminer::handle_genjob)
      HANDLE_NOTIFY_T2(tools::COMMAND_RPC_GET_FULLSCRATCHPAD, &poolminer::handle_get_full_scratchpad)
      HANDLE_NOTIFY_T2(tools::COMMAND_RPC_SUBMITSHARE, &poolminer::handle_submit_share)
      
    END_INVOKE_MAP2()

    //----------------- commands handlers ----------------------------------------------
    int handle_login(int command,  tools::COMMAND_RPC_LOGIN::request& arg, pool_connection_context& context);
    int handle_genjob(int command, tools::COMMAND_RPC_GETJOB::request& arg, pool_connection_context& context);
    int handle_get_full_scratchpad(int command, tools::COMMAND_RPC_GET_FULLSCRATCHPAD::request& arg, pool_connection_context& context);
    int handle_submit_share(int command, tools::COMMAND_RPC_SUBMITSHARE::request& arg, pool_connection_context& context);

    net_server m_net_server;
    volatile uint32_t m_stop;
    ::critical_section m_template_lock;
    block m_template;
    difficulty_type m_diffic;
    uint64_t m_height;
    i_miner_handler* m_phandler;
    account_public_address m_mine_address;
    math_helper::once_a_time_seconds<20> m_update_block_template_interval;
    pool_credentials m_pool_credentials; 
    blockchain_storage& m_bc;
    ::critical_section m_pool_credentials_lock;
  };
}



