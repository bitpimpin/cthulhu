
// Copyright 2017 Marco Cecconi
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "commands.h"
#include "../lib/redismodule.h"
#include "../lib/duktape.h"

duk_ret_t list_push(duk_context *_ctx) {
  const char * key = duk_require_string(_ctx, 0); // key name
  duk_bool_t where = duk_require_boolean(_ctx, 1); // true: head, false: tail
  const char * value = duk_to_string(_ctx, 2); // value

  RedisModuleString *RMS_Key = RedisModule_CreateString(RM_ctx, key, strlen(key));
  RedisModuleString *RMS_Value = RedisModule_CreateString(RM_ctx, value, strlen(value));

  void *key_h = RedisModule_OpenKey(RM_ctx, RMS_Key, REDISMODULE_WRITE);
  
  int ret = RedisModule_ListPush(key_h, (where ? REDISMODULE_LIST_HEAD : REDISMODULE_LIST_TAIL), RMS_Value);

  if (auto_replication) {
    int res = RedisModule_Replicate(RM_ctx, where ? "LPUSH":"RPUSH", "ss", RMS_Key, RMS_Value);
    if (res == REDISMODULE_ERR) {
      RedisModule_Log(RM_ctx, "error", "replication failed");
      RedisModule_CloseKey(key_h);
      RedisModule_FreeString(RM_ctx, RMS_Key);
      RedisModule_FreeString(RM_ctx, RMS_Value);
      return duk_error(_ctx, DUK_ERR_TYPE_ERROR, "replication failed");
    }
  }


  RedisModule_CloseKey(key_h);
  duk_pop(_ctx);

  RedisModule_FreeString(RM_ctx, RMS_Key);
  RedisModule_FreeString(RM_ctx, RMS_Value);

  duk_push_boolean(_ctx, ret == REDISMODULE_OK);
  return 1;
}

duk_ret_t list_pop(duk_context *_ctx) {

  const char * key = duk_require_string(_ctx, 0); // key name
  duk_bool_t where = duk_require_boolean(_ctx, 1); // true: head, false: tail

  RedisModuleString *RMS_Key = RedisModule_CreateString(RM_ctx, key, strlen(key));

  void *key_h = RedisModule_OpenKey(RM_ctx, RMS_Key, REDISMODULE_WRITE);
  
  RedisModuleString *RMS_Value = RedisModule_ListPop(key_h, (where ? REDISMODULE_LIST_HEAD : REDISMODULE_LIST_TAIL));

  if (auto_replication) {
    int res = RedisModule_Replicate(RM_ctx, where ? "LPOP":"RPOP", "s", RMS_Key);
    if (res == REDISMODULE_ERR) {
      RedisModule_Log(RM_ctx, "error", "replication failed");
      RedisModule_CloseKey(key_h);
      RedisModule_FreeString(RM_ctx, RMS_Key);
      RedisModule_FreeString(RM_ctx, RMS_Value);
      return duk_error(_ctx, DUK_ERR_TYPE_ERROR, "replication failed");
    }
  }
  RedisModule_CloseKey(key_h);

  size_t len;
  duk_push_string(_ctx, RedisModule_StringPtrLen(RMS_Value, &len));

  RedisModule_FreeString(RM_ctx, RMS_Key);
  RedisModule_FreeString(RM_ctx, RMS_Value);

  return 1;
}
