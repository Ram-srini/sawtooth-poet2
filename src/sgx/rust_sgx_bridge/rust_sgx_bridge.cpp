/*
 Copyright 2018 Intel Corporation

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <iostream>
#include "poet_enclave_u.h"
#include "sgx_urts.h"
#include "rust_sgx_bridge.h"
#include "poet_enclave.h"
#include "c11_support.h"
#include "error.h"
#include <iostream>
#include "poet.h"

#define DURATION_LENGTH_BYTES 8 //Duration to return is 64 bits (8 bytes)

Poet* Poet::instance = 0;

WaitCertificate* validate_wait_certificate(const char* ser_wait_cert, 
              const char* ser_wait_cert_sig);

int r_initialize_enclave(r_sgx_enclave_id_t *eid, const char * enclave_path, 
                         const char *spid) 
{
    //check parameters
    if (!eid || !enclave_path || !spid) {
        return -1;
    }
    try{
        Poet *poet_enclave_id = Poet::getInstance(enclave_path, spid);
        //store the enclave id
        eid->handle = (intptr_t)poet_enclave_id;
    } catch( sawtooth::poet::PoetError& e) {   
        return -1;
    }
    return 0;
}

int r_free_enclave(r_sgx_enclave_id_t *eid)
{
    if (eid->handle != 0) {
      Poet *poet_enclave_id = (Poet *)eid->handle;   
      delete poet_enclave_id;   
      eid->handle = 0;
    }

    return 0;
}

int r_create_signup_info(r_sgx_enclave_id_t *eid, const char *opk_hash, 
                         r_sgx_signup_info_t *signup_info) {
	
    if (!eid || !opk_hash || !signup_info) {
        return -1;
    }

    if (eid->handle == 0) {
        return -1;
    }
  
    _SignupData *signup_data = _create_signup_data(opk_hash);
    if (signup_data == NULL) {
        return -1;
    }
    //store the _SignupData handle
    signup_info->handle = (intptr_t)signup_data; 

    if (signup_data->poet_public_key.empty()) {
      return -1;
    }
    signup_info->poet_public_key = (char *)signup_data->poet_public_key.c_str();

    return 0;   
}


int r_initialize_wait_certificate(r_sgx_enclave_id_t *eid, uint8_t* duration, 
                                  const char* prev_cert, const char* prev_block_id, 
                                  const char* poet_block_id, const char* validator_id) {
    if (!eid || (prev_cert == NULL) || (prev_block_id == NULL) 
          || (validator_id == NULL) || (poet_block_id == NULL)) {
       return -1;
    }

    if (eid-> handle == 0) {
        return -1;
    }

    poet_err_t ret = initialize_wait_certificate(prev_cert, prev_block_id,
                                                 poet_block_id, validator_id, 
                                                 duration, DURATION_LENGTH_BYTES); 
    return ret;
}

int r_finalize_wait_certificate(r_sgx_enclave_id_t* eid, 
                                r_sgx_wait_certificate_t* wait_cert, 
                                const char* prev_block_id, 
                                const char* block_summary,
                                uint64_t wait_time) {

    if (!eid || (prev_block_id == NULL) || (block_summary == NULL)) {
        return -1;
    }
    
    if ( (eid->handle == 0)) {
        return -1;
    }

    WaitCertificate *wait_certificate = finalize_wait_certificate(prev_block_id, 
                                                                 block_summary,
                                                                 wait_time);                                                                 
    if (wait_certificate == NULL) {
      return -1;
    }
    //store wait certificate handle
    wait_cert->handle = (intptr_t) wait_certificate;
    if ( wait_certificate->serialized.empty() ) {
        return -1;
    }
    wait_cert->ser_wait_cert = (char*) wait_certificate->serialized.c_str();
    if (wait_certificate->signature.empty()) {
        return -1;
    }
    wait_cert->ser_wait_cert_sign = (char*) wait_certificate->signature.c_str();   
    return 0;
}


WaitCertificate* validate_wait_certificate(const char* ser_wait_cert, 
                                           const char* ser_wait_cert_sig) {
    
    return deserialize_wait_certificate(ser_wait_cert, 
                                          ser_wait_cert_sig);
}



int r_release_signup_info(r_sgx_enclave_id_t *eid, r_sgx_signup_info_t *signup_info)
{
    if (!eid || !signup_info) {
        return -1;
    }

    if (eid->handle == 0) {
       return -1;
    }

    if (signup_info->handle == 0) {
       return -1;
    }

    _SignupData *signup_data = (_SignupData *)signup_info->handle;
    if (signup_data != NULL) {
        _destroy_signup_data(signup_data);
        signup_info->handle = 0;
    }    
    return 0;
}

int r_release_wait_certificate(r_sgx_enclave_id_t *eid, 
                               r_sgx_wait_certificate_t *wait_cert)
{
    if (!eid || !wait_cert) {
        return -1;
    }
    
    if ((eid->handle == 0) || (wait_cert->handle == 0)) {
        return -1;
    }

    WaitCertificate *wait_certificate = (WaitCertificate *)wait_cert->handle;
    if (wait_certificate != NULL) {
        _destroy_wait_certificate(wait_certificate);
        wait_cert->handle = 0;
    }
    return 0;
}
