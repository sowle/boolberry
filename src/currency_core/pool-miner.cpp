
#include <sstream>
#include <boost/utility/value_init.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/limits.hpp>
#include <boost/foreach.hpp>
#include "misc_language.h"
#include "include_base_utils.h"
#include "currency_basic_impl.h"
#include "currency_format_utils.h"
#include "file_io_utils.h"
#include "time_helper.h"

using namespace epee;

#include "pool-miner.h"

namespace currency
{
  poolminer::poolminer(i_miner_handler* phandler, blockchain_storage& bc):
                 m_stop(1),
                 m_template(boost::value_initialized<block>()),
                 m_diffic(0),
                 m_phandler(phandler),
                 m_height(0), 
                 m_bc(bc)
  {

  }
  //-----------------------------------------------------------------------------------------------------
  poolminer::~poolminer()
  {
    stop();
    m_net_server.send_stop_signal();
    m_net_server.timed_wait_server_stop(1000);
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::set_block_template(const block& bl, const difficulty_type& di, uint64_t height)
  {
    CRITICAL_REGION_LOCAL(m_template_lock);
    m_template = bl;
    m_diffic = di;
    m_height = height;

    tools::NOTIFY_RPC_UPDATE_JOB::request req = AUTO_VAL_INIT(req); 

    m_net_server.get_config_object().foreach_connection([&](pool_connection_context& pcc)
    {
      notify_remote_command2(pcc.m_connection_id, tools::NOTIFY_RPC_UPDATE_JOB::ID, req, m_net_server.get_config_object());
      return true;
    });
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  void poolminer::init_options(boost::program_options::options_description& desc_cmd_only)
  {
    command_line::add_arg(desc_cmd_only, arg_pool_credentials_file);
    command_line::add_arg(desc_cmd_only, arg_pool_listen_port);
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::on_block_chain_update()
  {
    return request_block_template();
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::request_block_template()
  {
    block bl = AUTO_VAL_INIT(bl);
    difficulty_type di = AUTO_VAL_INIT(di);
    uint64_t height = AUTO_VAL_INIT(height);
    currency::blobdata exta_for_block = "@#@                ";//reserve for user_id and session_id
    if(!m_phandler->get_block_template(bl, m_mine_address, di, height, exta_for_block, true, currency::alias_info()))
    {
      LOG_ERROR("Failed to get_block_template(), stopping mining");
      stop();
      return false;
    }
    set_block_template(bl, di, height);
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::on_idle()
  {
    m_update_block_template_interval.do_call([&](){
      request_block_template();
      return true;
    });
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::init(const boost::program_options::variables_map& vm)
  {
    m_net_server.set_threads_prefix("POOL");
    m_net_server.get_config_object().m_pcommands_handler = this;
    m_net_server.get_config_object().m_invoke_timeout = POOL_DEFAULT_INVOKE_TIMEOUT;

   if(command_line::has_arg(vm, arg_pool_listen_port))
   {
     CHECK_AND_ASSERT_MES(command_line::has_arg(vm, arg_pool_credentials_file), false, "pool-credentials-file parameter not set"); 

     LOG_PRINT_L0("Inintializaing build-in pool...");
     LOG_PRINT_L0("Binding 0.0.0.0:" << command_line::get_arg(vm, arg_pool_listen_port));
     bool res = m_net_server.init_server(std::to_string(command_line::get_arg(vm, arg_pool_listen_port)));
     CHECK_AND_ASSERT_MES(res, false, "Failed to bind pool server");
     uint32_t listenning_port = m_net_server.get_binded_port();
     LOG_PRINT_L0("Pool net service binded on 0.0.0.0" << ":" << listenning_port);
   }

   return true;
  }
  //-----------------------------------------------------------------------------------------------------
  int poolminer::handle_submit_share(int command, tools::COMMAND_RPC_SUBMITSHARE::request& arg, pool_connection_context& context)
  {
    block bl = AUTO_VAL_INIT(bl);
    return 1;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::is_running()
  {
    return m_net_server.get_binded_port() != 0;
  }
  //-----------------------------------------------------------------------------------------------------
  void poolminer::send_stop_signal()
  {
    m_net_server.send_stop_signal();
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::start()
  {
    if(!is_running())
      return true;

    LOG_PRINT_L0("Starting pool server....");
    bool res = m_net_server.run_server(15, false);
    CHECK_AND_ASSERT_MES(res, false, "Failed to run_server for builtin pool");
    LOG_PRINT_L0("Pool server started ok");
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::stop()
  {
    m_net_server.send_stop_signal();
    return m_net_server.timed_wait_server_stop(100000);
  }
  //-----------------------------------------------------------------------------------------------------
}

