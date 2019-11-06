/*
 * Copyright 2019 Google
 *
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

#include "Firestore/Protos/nanopb/firestore/local/maybe_document.nanopb.h"
#include "Firestore/Protos/nanopb/firestore/local/mutation.nanopb.h"
#include "Firestore/Protos/nanopb/firestore/local/target.nanopb.h"
#include "Firestore/Protos/nanopb/google/firestore/v1/document.nanopb.h"
#include "Firestore/Protos/nanopb/google/firestore/v1/firestore.nanopb.h"
#include "Firestore/Protos/nanopb/google/firestore/v1/write.nanopb.h"
#include "Firestore/core/src/firebase/firestore/nanopb/message.h"
#include "Firestore/core/src/firebase/firestore/nanopb/nanopb_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace firebase {
namespace firestore {
namespace nanopb {
namespace {

using ::testing::MatchesRegex;

TEST(PrettyPrintingTest, PrintsInt) {
  Message<firestore_client_WriteBatch> m;
  m->batch_id = 123;

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<WriteBatch 0x[0-9A-Fa-f]+>: {
  batch_id: 123
})"));
}

TEST(PrettyPrintingTest, PrintsBool) {
  Message<firestore_client_MaybeDocument> m;
  m->has_committed_mutations = true;

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<MaybeDocument 0x[0-9A-Fa-f]+>: {
  has_committed_mutations: true
})"));
}

TEST(PrettyPrintingTest, PrintsString) {
  Message<firestore_client_MutationQueue> m;
  m->last_stream_token = MakeBytesArray("Abc123");

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<MutationQueue 0x[0-9A-Fa-f]+>: {
  last_stream_token: "Abc123"
})"));
}

TEST(PrettyPrintingTest, PrintsBytes) {
  Message<firestore_client_MutationQueue> m;
  m->last_stream_token = MakeBytesArray("\001\002\003");

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<MutationQueue 0x[0-9A-Fa-f]+>: {
  last_stream_token: "\\001\\002\\003"
})"));
}

TEST(PrettyPrintingTest, PrintsEnums) {
  Message<google_firestore_v1_TargetChange> m;
  m->target_change_type =
      google_firestore_v1_TargetChange_TargetChangeType_CURRENT;

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<TargetChange 0x[0-9A-Fa-f]+>: {
  target_change_type: CURRENT
})"));
}

TEST(PrettyPrintingTest, PrintsSubmessages) {
  Message<firestore_client_Target> m;
  m->snapshot_version.seconds = 123;
  m->snapshot_version.nanos = 456;

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<Target 0x[0-9A-Fa-f]+>: {
  snapshot_version {
    seconds: 123
    nanos: 456
  }
})"));
}

TEST(PrettyPrintingTest, PrintsArraysOfPrimitives) {
  Message<google_firestore_v1_Target_DocumentsTarget> m;

  m->documents_count = 2;
  m->documents = MakeArray<pb_bytes_array_t*>(m->documents_count);
  m->documents[0] = MakeBytesArray("doc1");
  m->documents[1] = MakeBytesArray("doc2");

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<DocumentsTarget 0x[0-9A-Fa-f]+>: {
  documents: "doc1"
  documents: "doc2"
})"));
}

TEST(PrettyPrintingTest, PrintsArraysOfObjects) {
  Message<google_firestore_v1_ListenRequest> m;

  m->labels_count = 2;
  m->labels =
      MakeArray<google_firestore_v1_ListenRequest_LabelsEntry>(m->labels_count);

  m->labels[0].key = MakeBytesArray("key1");
  m->labels[0].value = MakeBytesArray("value1");
  m->labels[1].key = MakeBytesArray("key2");
  m->labels[1].value = MakeBytesArray("value2");

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<ListenRequest 0x[0-9A-Fa-f]+>: {
  labels {
    key: "key1"
    value: "value1"
  }
  labels {
    key: "key2"
    value: "value2"
  }
})"));
}

TEST(PrettyPrintingTest, PrintsPrimitivesInOneofs) {
  Message<google_firestore_v1_Write> m;
  m->which_operation = google_firestore_v1_Write_delete_tag;
  // Also checks for the special case with `delete` being a keyword in C++.
  m->delete_ = MakeBytesArray("abc");

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<Write 0x[0-9A-Fa-f]+>: {
  delete: "abc"
})"));
}

TEST(PrettyPrintingTest, PrintsMessagesInOneofs) {
  // This test also exercises deeply-nested messages.
  Message<google_firestore_v1_Write> m;
  m->which_operation = google_firestore_v1_Write_update_tag;

  auto& doc = m->update;
  doc.name = MakeBytesArray("some name");

  doc.fields_count = 2;
  doc.fields =
      MakeArray<google_firestore_v1_Document_FieldsEntry>(doc.fields_count);

  // Also checks that even fields with default values are printed if they're the
  // active member of a oneof.
  doc.fields[0].key = MakeBytesArray("key1");
  doc.fields[0].value.which_value_type =
      google_firestore_v1_Value_boolean_value_tag;
  doc.fields[0].value.boolean_value = false;

  doc.fields[1].key = MakeBytesArray("key2");
  doc.fields[1].value.which_value_type =
      google_firestore_v1_Value_timestamp_value_tag;

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<Write 0x[0-9A-Fa-f]+>: {
  update {
    name: "some name"
    fields {
      key: "key1"
      value {
        boolean_value: false
      }
    }
    fields {
      key: "key2"
      value {
        timestamp_value {
        }
      }
    }
  }
})"));
}

TEST(PrettyPrintingTest, PrintsNonAnonymousOneofs) {
  Message<google_firestore_v1_RunQueryRequest> m;

  m->which_consistency_selector =
      google_firestore_v1_RunQueryRequest_read_time_tag;
  m->consistency_selector.read_time.seconds = 123;
  m->consistency_selector.read_time.nanos = 456;
  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<RunQueryRequest 0x[0-9A-Fa-f]+>: {
  read_time {
    seconds: 123
    nanos: 456
  }
})"));
}

TEST(PrettyPrintingTest, PrintsOptionals) {
  Message<google_firestore_v1_Write> m;

  auto& mask = m->update_mask;
  mask.field_paths_count = 2;
  mask.field_paths = MakeArray<pb_bytes_array_t*>(mask.field_paths_count);
  mask.field_paths[0] = MakeBytesArray("abc");
  mask.field_paths[1] = MakeBytesArray("def");

  // `has_update_mask` is false, so `update_mask` shouldn't be printed.
  // Note that normally setting `update_mask` without setting `has_update_mask`
  // to true shouldn't happen.
  EXPECT_THAT(m.ToString(), MatchesRegex("<Write 0x[0-9A-Fa-f]+>: {\n}"));

  m->has_update_mask = true;
  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<Write 0x[0-9A-Fa-f]+>: {
  update_mask {
    field_paths: "abc"
    field_paths: "def"
  }
})"));
}

TEST(PrettyPrintingTest, PrintsEmptyArrayElements) {
  Message<google_firestore_v1_Target_DocumentsTarget> m;

  m->documents_count = 2;
  m->documents = MakeArray<pb_bytes_array_t*>(m->documents_count);
  m->documents[0] = MakeBytesArray("");
  m->documents[1] = MakeBytesArray("");

  EXPECT_THAT(m.ToString(), MatchesRegex(
                                R"(<DocumentsTarget 0x[0-9A-Fa-f]+>: {
  documents: ""
  documents: ""
})"));
}

TEST(PrettyPrintingTest, PrintsEmptyMessageIfRoot) {
  Message<google_firestore_v1_Write> m;
  EXPECT_THAT(m.ToString(), MatchesRegex("<Write 0x[0-9A-Fa-f]+>: {\n}"));
}

}  //  namespace
}  //  namespace nanopb
}  //  namespace firestore
}  //  namespace firebase
