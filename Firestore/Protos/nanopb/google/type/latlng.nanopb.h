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

#ifndef PB_GOOGLE_TYPE_LATLNG_NANOPB_H_INCLUDED
#define PB_GOOGLE_TYPE_LATLNG_NANOPB_H_INCLUDED
#include <pb.h>

#include <string>

namespace firebase {
namespace firestore {
namespace nanopb {

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif


/* Struct definitions */
typedef struct _google_type_LatLng {
    double latitude;
    double longitude;

    std::string ToString(int indent = 0) const;
/* @@protoc_insertion_point(struct:google_type_LatLng) */
} google_type_LatLng;

/* Default values for struct fields */

/* Initializer values for message structs */
#define google_type_LatLng_init_default          {0, 0}
#define google_type_LatLng_init_zero             {0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define google_type_LatLng_latitude_tag          1
#define google_type_LatLng_longitude_tag         2

/* Struct field encoding specification for nanopb */
extern const pb_field_t google_type_LatLng_fields[3];

/* Maximum encoded size of messages (where known) */
#define google_type_LatLng_size                  18

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define LATLNG_MESSAGES \


#endif

}  // namespace nanopb
}  // namespace firestore
}  // namespace firebase

/* @@protoc_insertion_point(eof) */

#endif
