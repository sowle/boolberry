#pragma once 
#include "serialization/keyvalue_serialization.h"


#define POOL_COMMANDS_BASE 4000

/************************************************************************/
/*                                                                      */
/************************************************************************/
namespace tools
{
  struct height_info
  {
    uint64_t height;
    crypto::hash block_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(height)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(block_id)
    END_KV_SERIALIZE_MAP()
  };

  struct job_details
  {
    std::string blob;
    currency::difficulty_type difficulty;
    std::string job_id;
    height_info prev_hi;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(blob)
      KV_SERIALIZE(difficulty)
      KV_SERIALIZE(job_id)
      KV_SERIALIZE(prev_hi)
    END_KV_SERIALIZE_MAP()
  };

  struct addendum
  {
    height_info  hi;
    crypto::hash prev_id;
    std::vector<crypto::hash>  addm;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(hi)
      KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(prev_id)
      KV_SERIALIZE_CONTAINER_POD_AS_BLOB(addm)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_LOGIN
  {
    const static int ID = POOL_COMMANDS_BASE + 1;

    struct request
    {
      std::string login;
      std::string pass;
      std::string agent;
      height_info hi;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(login)
        KV_SERIALIZE(pass)
        KV_SERIALIZE(agent)
        KV_SERIALIZE(hi)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::string id;
      job_details job;
      std::list<addendum> addms;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(id)
        KV_SERIALIZE(job)
        KV_SERIALIZE(addms)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GETJOB
  {
    const static int ID = POOL_COMMANDS_BASE + 2;

    struct request
    {
      std::string id;
      height_info hi;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
        KV_SERIALIZE(hi)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      job_details jd;
      std::list<addendum> addms;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jd)
        KV_SERIALIZE(addms)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_FULLSCRATCHPAD
  {
    const static int ID = POOL_COMMANDS_BASE + 3;

    struct request
    {
      std::string id;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      height_info hi;
      std::string scratchpad_hex;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hi)
        KV_SERIALIZE(scratchpad_hex)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_SUBMITSHARE
  {
    const static int ID = POOL_COMMANDS_BASE + 4;

    struct request
    {
      std::string id;
      uint64_t nonce;
      std::string job_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
        KV_SERIALIZE(nonce)
        KV_SERIALIZE(job_id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct NOTIFY_RPC_UPDATE_JOB
  {
    const static int ID = POOL_COMMANDS_BASE + 4;

    struct request
    {
      job_details jd;
      std::list<addendum> addms;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jd)
        KV_SERIALIZE(addms)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
}
