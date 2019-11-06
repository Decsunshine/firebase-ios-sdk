/*
 * Copyright 2018 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.2 */

#ifndef PB_FIRESTORE_CLIENT_MUTATION_NANOPB_H_INCLUDED
#define PB_FIRESTORE_CLIENT_MUTATION_NANOPB_H_INCLUDED
#include <pb.h>

#include "google/firestore/v1/write.nanopb.h"

#include "google/protobuf/timestamp.nanopb.h"

#include <string>

namespace firebase {
namespace firestore {
namespace nanopb {

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif


/* Struct definitions */
typedef struct _firestore_client_MutationQueue {
    int32_t last_acknowledged_batch_id;
    pb_bytes_array_t *last_stream_token;

    std::string ToString(int indent = 0) const;
/* @@protoc_insertion_point(struct:firestore_client_MutationQueue) */
} firestore_client_MutationQueue;

typedef struct _firestore_client_WriteBatch {
    int32_t batch_id;
    pb_size_t writes_count;
    struct _google_firestore_v1_Write *writes;
    google_protobuf_Timestamp local_write_time;
    pb_size_t base_writes_count;
    struct _google_firestore_v1_Write *base_writes;

    std::string ToString(int indent = 0) const;
/* @@protoc_insertion_point(struct:firestore_client_WriteBatch) */
} firestore_client_WriteBatch;

/* Default values for struct fields */

/* Initializer values for message structs */
#define firestore_client_MutationQueue_init_default {0, NULL}
#define firestore_client_WriteBatch_init_default {0, 0, NULL, google_protobuf_Timestamp_init_default, 0, NULL}
#define firestore_client_MutationQueue_init_zero {0, NULL}
#define firestore_client_WriteBatch_init_zero    {0, 0, NULL, google_protobuf_Timestamp_init_zero, 0, NULL}

/* Field tags (for use in manual encoding/decoding) */
#define firestore_client_MutationQueue_last_acknowledged_batch_id_tag 1
#define firestore_client_MutationQueue_last_stream_token_tag 2
#define firestore_client_WriteBatch_batch_id_tag 1
#define firestore_client_WriteBatch_writes_tag   2
#define firestore_client_WriteBatch_local_write_time_tag 3
#define firestore_client_WriteBatch_base_writes_tag 4

/* Struct field encoding specification for nanopb */
extern const pb_field_t firestore_client_MutationQueue_fields[3];
extern const pb_field_t firestore_client_WriteBatch_fields[5];

/* Maximum encoded size of messages (where known) */
/* firestore_client_MutationQueue_size depends on runtime parameters */
/* firestore_client_WriteBatch_size depends on runtime parameters */

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define MUTATION_MESSAGES \


#endif

}  // namespace nanopb
}  // namespace firestore
}  // namespace firebase

/* @@protoc_insertion_point(eof) */

#endif
