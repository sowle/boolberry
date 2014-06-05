
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
#include "version.h"

using namespace epee;

#include "pool-miner.h"

#define POOL_MINER_RESERVATION_PREFIX   "|@#@"
#define POOL_MINER_RESERVATION          "                "


namespace currency
{
  std::string get_session_str(const pool_connection_context& context)
  {
    std::stringstream ss;
    ss<< "[" << context.user_login << "@" << string_tools::get_ip_string_from_int32(context.m_remote_ip) << "|" << context.session_id << "]";
    return ss.str();
  }

#define CHECK_SESSION()     if(!context.session_id) \
  { \
    rsp.status = "Session is not initialized"; \
    return 1; \
  } 


  poolminer::poolminer(i_miner_handler* phandler, blockchain_storage& bc):
                 m_stop(1),
                 m_template(boost::value_initialized<block>()),
                 m_diffic(0),
                 m_template_ver(0),
                 m_phandler(phandler),
                 m_height(0), 
                 m_bc(bc), 
                 m_ssession_counter(1), 
                 m_do_donations(true)
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
    CRITICAL_REGION_BEGIN(m_template_lock);
    m_template = bl;
    m_diffic = di;
    m_height = height;
    m_template_ver++;
    CRITICAL_REGION_END();

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
  bool poolminer::update_scratchpad()
  {
    EXCLUSIVE_CRITICAL_REGION_LOCAL(m_scratchpad_access);
    return m_bc.copy_scratchpad(m_scratchpad);
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::on_block_chain_update()
  {
    if(!is_running())
      return true;

    update_scratchpad();
    //validate_alias_info();
    return request_block_template();
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::request_block_template()
  {
    block bl = AUTO_VAL_INIT(bl);
    difficulty_type di = AUTO_VAL_INIT(di);
    uint64_t height = AUTO_VAL_INIT(height);
    currency::blobdata exta_for_block = PROJECT_VERSION_LONG POOL_MINER_RESERVATION_PREFIX POOL_MINER_RESERVATION; //reserve for user_id and session_id
    if(!m_bc.create_block_template(bl, m_mine_address, di, height, exta_for_block, true, currency::alias_info()))
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
     m_credentials_filepath = command_line::get_arg(vm, arg_pool_credentials_file);

     if(command_line::has_arg(vm, command_line::arg_set_donation_mode))
     {
       std::string desc = command_line::get_arg(vm, command_line::arg_set_donation_mode);
       CHECK_AND_ASSERT_MES(desc == "true" || desc == "false", false, "wrong donation mode option");

       if( desc == "true")
         m_do_donations = true;
       else 
         m_do_donations = false;       
     }

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
  bool poolminer::reload_credentials()
  {    
    pool_credentials pc;
    bool r = epee::serialization::load_t_from_json_file(pc, m_credentials_filepath);
    CHECK_AND_ASSERT_MES(r, false, "Failed to load credentials file: " << m_credentials_filepath);

    m_credentials.clear();

    r = currency::get_account_address_from_str(m_owner_address, pc.owner_address);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse owner address: " << pc.owner_address);
  
    for(auto& c: pc.credentials)
    {
      CHECK_AND_ASSERT_MES(m_credentials.find(c.login) == m_credentials.end(), false, "Double login in credentials file: " << c.login);
      credential_entry ce = AUTO_VAL_INIT(ce);
      r = currency::get_account_address_from_str(ce.address, c.address);
      CHECK_AND_ASSERT_MES(r, false, "Failed to parse credential address: " << c.address);
      m_credentials[c.login] = ce;
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::get_addendum_for_hi(const tools::height_info& hi, std::list<tools::addendum>& res)
  {
    res.clear();
    if(!hi.height || hi.height + 1 == m_bc.get_current_blockchain_height())
      return true;//do not make addendum for whole blockchain

    CHECK_AND_ASSERT_MES(hi.height < m_bc.get_current_blockchain_height(), false, "wrong height parameter passed: " << hi.height);

    crypto::hash block_chain_id = m_bc.get_block_id_by_height(hi.height);
    CHECK_AND_ASSERT_MES(block_chain_id != null_hash, false, "internal error: can't get block id by height: " << hi.height);
    uint64_t height = hi.height;
    if(block_chain_id != hi.block_id)
    {
      //probably split
      CHECK_AND_ASSERT_MES(hi.height > 0, false, "wrong height passed");
      --height;
    }

    std::list<block> blocks;
    bool r = m_bc.get_blocks(height + 1, m_bc.get_current_blockchain_height() - (height+1), blocks);
    CHECK_AND_ASSERT_MES(r, false, "failed to get blocks");
    for(auto it = blocks.begin(); it!= blocks.end(); it++)
    {
      res.push_back(tools::addendum());
      res.back().hi.height = get_block_height(*it);
      res.back().hi.block_id = get_block_hash(*it);
      res.back().prev_id = it->prev_id;      
      r = get_block_scratchpad_addendum(*it, res.back().addm);
      CHECK_AND_ASSERT_MES(r, false, "Failed to add block addendum");
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::update_context_template(pool_connection_context& cntx)
  {
    if(cntx.template_ver != m_template_ver)
    {
      CRITICAL_REGION_BEGIN(m_template_lock);
      cntx.bl_templ = m_template;
      cntx.block_diff = m_diffic;
      cntx.share_diff = m_diffic/POOL_SHARE_DIFFICULTY_RATIO;
      cntx.template_height = m_height;
      CRITICAL_REGION_END();
      
      CHECK_AND_ASSERT_MES(cntx.bl_templ.miner_tx.extra.size() >= strlen(PROJECT_VERSION_LONG POOL_MINER_RESERVATION_PREFIX POOL_MINER_RESERVATION), 
                          false, 
                          "Wrong template reservation prefix size = " << cntx.bl_templ.miner_tx.extra.size() << ", expected: " << strlen(PROJECT_VERSION_LONG POOL_MINER_RESERVATION_PREFIX POOL_MINER_RESERVATION));

      //patch template
      *reinterpret_cast<uint64_t*>(&cntx.bl_templ.miner_tx.extra[strlen(PROJECT_VERSION_LONG POOL_MINER_RESERVATION_PREFIX)]) = cntx.session_id;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::get_job_for_sessoin(tools::job_details& job, pool_connection_context& cntx)
  {
    if(!cntx.last_assigned_hi.height)
      return true;
    update_context_template(cntx);
    if(cntx.last_assigned_hi.height != cntx.template_height - 1 )
    {
      LOG_PRINT_L0("Last assigned height " << cntx.last_assigned_hi.height << " mismatched with template height: " << cntx.template_height - 1);
      return false;
    }
    job.blob = get_block_hashing_blob(cntx.bl_templ);
    job.difficulty = cntx.share_diff;
    job.job_id = "";//leave it blank
    job.prev_hi.block_id = cntx.bl_templ.prev_id;
    job.prev_hi.height = cntx.template_height;
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::get_job_and_addendum(const tools::height_info& hi,  
                                       std::list<tools::addendum>& addms, 
                                       tools::job_details& job, 
                                       pool_connection_context& context, 
                                       std::string& err_status)
  {
    size_t tries_count = 0;
    for(; tries_count != 3; tries_count++)
    {
      if(!get_addendum_for_hi(hi, addms))
      {
        err_status = "Fail at get_addendum_for_hi, check daemon logs for details";
        return true;
      }

      if(addms.size())
        context.last_assigned_hi = addms.back().hi;
      else
        context.last_assigned_hi = hi;

      if(hi.height)
      {
        if(!get_job_for_sessoin(job, context))
        {
          LOG_PRINT_L0("failed to get job for session...");
          continue;
        }
      }
    }
    if(tries_count == 3)
    {
      err_status = "Failed to create get_job.";
      return false;
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  int poolminer::handle_login(int command,  tools::COMMAND_RPC_LOGIN::request& arg, tools::COMMAND_RPC_LOGIN::response& rsp, pool_connection_context& context)
  {
    CRITICAL_REGION_LOCAL(m_credentials_lock);
    auto it = m_credentials.find(arg.login);

    if(it == m_credentials.end())
    {
      LOG_PRINT_RED_L0("Wrong auth info, login " << arg.login << " not found");
      m_net_server.get_config_object().close(context.m_connection_id);
      return 1;
    }

    //check password
    if(it->second.password != arg.pass)
    {
      LOG_PRINT_RED_L0("Wrong auth info, login " << arg.login << " passed wrong password");
      m_net_server.get_config_object().close(context.m_connection_id);
      return 1;
    }      

    context.session_id = m_ssession_counter++;
    context.user_login = arg.login;    
    rsp.session_id = std::to_string(context.session_id);
    if(!get_job_and_addendum(arg.hi, rsp.addms, rsp.job, context, rsp.status))
      return 1;

    LOG_PRINT_GREEN("[pool]: User \"" << arg.login << "\"logged in, ip: " << string_tools::get_ip_string_from_int32(context.m_remote_ip) 
                    << ", session_id=" << context.session_id 
                    << ", last_height=" << arg.hi.height , LOG_LEVEL_0);
    rsp.status = POOL_STATUS_OK;
    return 1;
  }
  //-----------------------------------------------------------------------------------------------------
  int poolminer::handle_genjob(int command, tools::COMMAND_RPC_GETJOB::request& arg, tools::COMMAND_RPC_GETJOB::response& rsp, pool_connection_context& context)
  {
    CHECK_SESSION();

    if(!get_job_and_addendum(arg.hi, rsp.addms, rsp.jd, context, rsp.status))
      return 1;

    LOG_PRINT_L1("[pool]: get job", LOG_LEVEL_0);

    rsp.status = POOL_STATUS_OK;
    return 1;
  }
  //-----------------------------------------------------------------------------------------------------
  int poolminer::handle_get_full_scratchpad(int command, tools::COMMAND_RPC_GET_FULLSCRATCHPAD::request& arg, tools::COMMAND_RPC_GET_FULLSCRATCHPAD::response& rsp, pool_connection_context& context)
  {
    CHECK_SESSION();
    LOG_PRINT_MAGENTA("[pool]" << get_session_str(context) << ": getting full scratchpad:", LOG_LEVEL_0);
    SHARED_CRITICAL_REGION_LOCAL(m_scratchpad_access);
    rsp.scratchpad = m_scratchpad;
    rsp.status = POOL_STATUS_OK;
    return 1;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::commit_share(const crypto::hash& h, const std::string& user_id)
  {
    CRITICAL_REGION_LOCAL(m_shares_lock);
    m_shares[h] = user_id;
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::is_share_already_sent(const crypto::hash& h)
  {
    CRITICAL_REGION_LOCAL(m_shares_lock);
    return m_shares.count(h);
  }
  //-----------------------------------------------------------------------------------------------------
  bool poolminer::next_round()
  {

  }
  //-----------------------------------------------------------------------------------------------------
  int poolminer::handle_submit_share(int command, tools::COMMAND_RPC_SUBMITSHARE::request& arg, tools::COMMAND_RPC_SUBMITSHARE::response& rsp, pool_connection_context& context)
  {
    CHECK_SESSION();
    //check share
    context.bl_templ.nonce = arg.nonce;
    crypto::hash pow_hash = null_hash;
    SHARED_CRITICAL_REGION_BEGIN(m_scratchpad_access);
    pow_hash = currency::get_block_longhash(context.bl_templ, context.template_height, m_scratchpad);
    CRITICAL_REGION_END();
    if(!currency::check_hash(pow_hash, context.share_diff))
    {
      LOG_PRINT_RED_L0("[pool]" << get_session_str(context) <<  "SENT WRONG SHARE!");
      rsp.status = "Failed: share have not enough difficulty";
      return 1;
    }

    if(is_share_already_sent(pow_hash))
    {
      LOG_PRINT_RED_L0("[pool]" << get_session_str(context) <<  "SENT WRONG SHARE(duplicate)!");
      rsp.status = "Failed: Duplicate share";
      return 1;
    }
    if(commit_share(pow_hash, context.user_login))
    {
      LOG_PRINT_RED_L0("[pool]" << get_session_str(context) <<  "Internal error: filed to commit share");
      rsp.status = "Internal error: filed to commit share";
      return 1;
    }
    
    LOG_PRINT_L0("[pool]" << get_session_str(context) <<  "sent share.");

    //check if this share is block
    if(currency::check_hash(pow_hash, context.block_diff))
    {
      LOG_PRINT_GREEN("[pool]" << get_session_str(context) <<  " Found block!", LOG_LEVEL_0);
      rsp.status = POOL_STATUS_OK;
      
      if(m_phandler->handle_block_found(context.bl_templ))
      {
        LOG_PRINT_GREEN("[pool]" << get_session_str(context) <<  " block accepted", LOG_LEVEL_0);
        bool next_round();
      }else
      {
        LOG_PRINT_RED("[pool]" << get_session_str(context) <<  " block rejected", LOG_LEVEL_0);
      }

      return 1;
    }

    rsp.status = POOL_STATUS_OK;
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

