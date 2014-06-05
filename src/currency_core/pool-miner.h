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

#define POOL_SHARE_DIFFICULTY_RATIO                     100
//#define POOL_MAX_USERS_ALIVE                            500
#define POOL_OWNER_FEE_PERCENTS                         3

namespace currency
{

  namespace
  {
    const command_line::arg_descriptor<std::string>         arg_pool_credentials_file = {"pool-credentials-file", "Setup credentials file here for builtin pool", "", false};
    const command_line::arg_descriptor<uint32_t>            arg_pool_listen_port = {"pool-bind-port", "Setup listening port for builtin pool", 0, false};
  }
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct pool_connection_context: public epee::net_utils::connection_context_base
  {
    std::string user_login;
    uint64_t session_id;
    tools::height_info last_assigned_hi;
    block bl_templ;
    uint64_t template_ver;
    difficulty_type share_diff;
    difficulty_type block_diff;
    uint64_t template_height;
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
    std::string owner_address;
    std::list<pool_credential> credentials;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(owner_address)
      KV_SERIALIZE(credentials)
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
      HANDLE_INVOKE_T2(tools::COMMAND_RPC_LOGIN, &poolminer::handle_login)
      HANDLE_INVOKE_T2(tools::COMMAND_RPC_GETJOB, &poolminer::handle_genjob)
      HANDLE_INVOKE_T2(tools::COMMAND_RPC_GET_FULLSCRATCHPAD, &poolminer::handle_get_full_scratchpad)
      HANDLE_INVOKE_T2(tools::COMMAND_RPC_SUBMITSHARE, &poolminer::handle_submit_share)
    END_INVOKE_MAP2()

    //----------------- commands handlers ----------------------------------------------
    int handle_login(int command,  tools::COMMAND_RPC_LOGIN::request& arg, tools::COMMAND_RPC_LOGIN::response& rsp, pool_connection_context& context);
    int handle_genjob(int command, tools::COMMAND_RPC_GETJOB::request& arg, tools::COMMAND_RPC_GETJOB::response& rsp, pool_connection_context& context);
    int handle_get_full_scratchpad(int command, tools::COMMAND_RPC_GET_FULLSCRATCHPAD::request& arg, tools::COMMAND_RPC_GET_FULLSCRATCHPAD::response& rsp, pool_connection_context& context);
    int handle_submit_share(int command, tools::COMMAND_RPC_SUBMITSHARE::request& arg, tools::COMMAND_RPC_SUBMITSHARE::response& rsp, pool_connection_context& context);
       
    bool reload_credentials();
    bool get_addendum_for_hi(const tools::height_info& hi, std::list<tools::addendum>& res);
    bool get_job_for_sessoin(tools::job_details& job, pool_connection_context& cntx);
    bool update_context_template(pool_connection_context& cntx);
    bool get_job_and_addendum(const tools::height_info& hi,  
                              std::list<tools::addendum>& addms, 
                              tools::job_details& job, 
                              pool_connection_context& context, 
                              std::string& err_status);
    bool update_scratchpad();
    bool commit_share(const crypto::hash& h, const std::string& user_id);
    bool is_share_already_sent(const crypto::hash& h);
    bool next_round();
    static bool calc_paymnets_from_shares(size_t total_shares_for_round, const std::map<std::string, uint64_t>& user_shares, std::list<std::pair<std::string, uint64_t> >& payments, uint64_t owner_payment);

    struct credential_entry
    {
      std::string password;
      account_public_address address;
    };


    net_server m_net_server;
    volatile uint32_t m_stop;
    ::critical_section m_template_lock;
    block m_template;
    difficulty_type m_diffic;
    uint64_t m_height;    
    std::atomic<uint64_t> m_template_ver;
    
    boost::shared_mutex m_scratchpad_access;
    std::vector<crypto::hash> m_scratchpad;

    i_miner_handler* m_phandler;
    math_helper::once_a_time_seconds<20> m_update_block_template_interval;    
    blockchain_storage& m_bc;
    bool m_do_donations;

    ::critical_section m_shares_lock;
    std::unordered_map<crypto::hash, std::string> m_shares;//share to user login

    ::critical_section m_current_payments_lock;
    std::list<std::pair<account_public_address, uint64_t> > m_current_payments;


    std::string m_credentials_filepath;
    ::critical_section m_credentials_lock;
    account_public_address m_owner_address;
    ::critical_section m_credentials_lock;
    std::map<std::string, credential_entry> m_credentials;

    std::atomic<uint64_t> m_ssession_counter;
  };
}



