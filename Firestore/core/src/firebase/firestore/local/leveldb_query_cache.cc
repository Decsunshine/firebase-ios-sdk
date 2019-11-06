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

#include "Firestore/core/src/firebase/firestore/local/leveldb_query_cache.h"

#include <string>
#include <utility>

#include "Firestore/core/src/firebase/firestore/local/leveldb_key.h"
#include "Firestore/core/src/firebase/firestore/local/leveldb_persistence.h"
#include "Firestore/core/src/firebase/firestore/local/leveldb_util.h"
#include "Firestore/core/src/firebase/firestore/local/local_serializer.h"
#include "Firestore/core/src/firebase/firestore/local/query_data.h"
#include "Firestore/core/src/firebase/firestore/local/reference_delegate.h"
#include "Firestore/core/src/firebase/firestore/model/document_key.h"
#include "Firestore/core/src/firebase/firestore/model/document_key_set.h"
#include "Firestore/core/src/firebase/firestore/nanopb/byte_string.h"
#include "Firestore/core/src/firebase/firestore/nanopb/reader.h"
#include "Firestore/core/src/firebase/firestore/util/string_apple.h"
#include "absl/strings/match.h"

namespace firebase {
namespace firestore {
namespace local {

using core::Query;
using leveldb::Status;
using model::DocumentKey;
using model::DocumentKeySet;
using model::ListenSequenceNumber;
using model::SnapshotVersion;
using model::TargetId;
using nanopb::ByteString;
using nanopb::firestore_client_Target;
using nanopb::firestore_client_TargetGlobal;
using nanopb::Message;
using nanopb::StringReader;
using util::MakeString;

absl::optional<Message<firestore_client_TargetGlobal>>
LevelDbQueryCache::TryReadMetadata(leveldb::DB* db) {
  std::string key = LevelDbTargetGlobalKey::Key();
  std::string value;
  Status status = db->Get(StandardReadOptions(), key, &value);

  StringReader reader{value};
  reader.set_status(ConvertStatus(status));

  auto result = Message<firestore_client_TargetGlobal>::TryParse(&reader);
  if (!reader.ok()) {
    if (reader.status().code() == Error::NotFound) {
      return absl::nullopt;
    } else {
      HARD_FAIL("ReadMetadata: failed loading key %s with status: %s", key,
                reader.status().ToString());
    }
  }

  return result;
}

Message<firestore_client_TargetGlobal> LevelDbQueryCache::ReadMetadata(
    leveldb::DB* db) {
  auto maybe_metadata = TryReadMetadata(db);
  if (!maybe_metadata) {
    HARD_FAIL(
        "Found no metadata, expected schema to be at version 0 which "
        "ensures metadata existence");
  }
  return std::move(maybe_metadata).value();
}

LevelDbQueryCache::LevelDbQueryCache(LevelDbPersistence* db,
                                     LocalSerializer* serializer)
    : db_(NOT_NULL(db)), serializer_(NOT_NULL(serializer)) {
}

void LevelDbQueryCache::Start() {
  // TODO(gsoltis): switch this usage of ptr to current_transaction()
  metadata_ = ReadMetadata(db_->ptr());

  StringReader reader;
  last_remote_snapshot_version_ = serializer_->DecodeVersion(
      &reader, metadata_->last_remote_snapshot_version);
  if (!reader.ok()) {
    HARD_FAIL("Failed to decode last remote snapshot version, reason: '%s'",
              reader.status().ToString());
  }
}

void LevelDbQueryCache::AddTarget(const QueryData& query_data) {
  Save(query_data);

  const std::string& canonical_id = query_data.query().CanonicalId();
  std::string index_key =
      LevelDbQueryTargetKey::Key(canonical_id, query_data.target_id());
  std::string empty_buffer;
  db_->current_transaction()->Put(index_key, empty_buffer);

  metadata_->target_count++;
  UpdateMetadata(query_data);
  SaveMetadata();
}

void LevelDbQueryCache::UpdateTarget(const QueryData& query_data) {
  Save(query_data);

  if (UpdateMetadata(query_data)) {
    SaveMetadata();
  }
}

void LevelDbQueryCache::RemoveTarget(const QueryData& query_data) {
  TargetId target_id = query_data.target_id();

  RemoveAllKeysForTarget(target_id);

  std::string key = LevelDbTargetKey::Key(target_id);
  db_->current_transaction()->Delete(key);

  std::string index_key =
      LevelDbQueryTargetKey::Key(query_data.query().CanonicalId(), target_id);
  db_->current_transaction()->Delete(index_key);

  metadata_->target_count--;
  SaveMetadata();
}

absl::optional<QueryData> LevelDbQueryCache::GetTarget(const Query& query) {
  // Scan the query-target index starting with a prefix starting with the given
  // query's canonical_id. Note that this is a scan rather than a get because
  // canonical_ids are not required to be unique per target.
  const std::string& canonical_id = query.CanonicalId();
  auto index_iterator = db_->current_transaction()->NewIterator();
  std::string index_prefix = LevelDbQueryTargetKey::KeyPrefix(canonical_id);
  index_iterator->Seek(index_prefix);

  // Simultaneously scan the targets table. This works because each
  // (canonical_id, target_id) pair is unique and ordered, so when scanning a
  // table prefixed by exactly one canonical_id, all the target_ids will be
  // unique and in order.
  std::string target_prefix = LevelDbTargetKey::KeyPrefix();
  auto target_iterator = db_->current_transaction()->NewIterator();

  LevelDbQueryTargetKey row_key;
  for (; index_iterator->Valid(); index_iterator->Next()) {
    // Only consider rows matching exactly the specific canonical_id of
    // interest.
    if (!absl::StartsWith(index_iterator->key(), index_prefix) ||
        !row_key.Decode(index_iterator->key()) ||
        canonical_id != row_key.canonical_id()) {
      // End of this canonical_id's possible targets.
      break;
    }

    // Each row is a unique combination of canonical_id and target_id, so this
    // foreign key reference can only occur once.
    std::string target_key = LevelDbTargetKey::Key(row_key.target_id());
    target_iterator->Seek(target_key);
    if (!target_iterator->Valid() || target_iterator->key() != target_key) {
      HARD_FAIL(
          "Dangling query-target reference found: "
          "%s points to %s; seeking there found %s",
          DescribeKey(index_iterator), DescribeKey(target_key),
          DescribeKey(target_iterator));
    }

    // Finally after finding a potential match, check that the query is actually
    // equal to the requested query.
    QueryData target = DecodeTarget(target_iterator->value());
    if (target.query() == query) {
      return target;
    }
  }

  return absl::nullopt;
}

void LevelDbQueryCache::EnumerateTargets(const TargetCallback& callback) {
  // Enumerate all targets, give their sequence numbers.
  std::string target_prefix = LevelDbTargetKey::KeyPrefix();
  auto it = db_->current_transaction()->NewIterator();
  it->Seek(target_prefix);
  for (; it->Valid() && absl::StartsWith(it->key(), target_prefix);
       it->Next()) {
    QueryData target = DecodeTarget(it->value());
    callback(target);
  }
}

int LevelDbQueryCache::RemoveTargets(
    ListenSequenceNumber upper_bound,
    const std::unordered_map<model::TargetId, QueryData>& live_targets) {
  int count = 0;
  std::string target_prefix = LevelDbTargetKey::KeyPrefix();
  auto it = db_->current_transaction()->NewIterator();
  it->Seek(target_prefix);
  for (; it->Valid() && absl::StartsWith(it->key(), target_prefix);
       it->Next()) {
    QueryData query_data = DecodeTarget(it->value());
    if (query_data.sequence_number() <= upper_bound &&
        live_targets.find(query_data.target_id()) == live_targets.end()) {
      RemoveTarget(query_data);
      count++;
    }
  }
  return count;
}

void LevelDbQueryCache::AddMatchingKeys(const DocumentKeySet& keys,
                                        TargetId target_id) {
  // Store an empty value in the index which is equivalent to serializing a
  // GPBEmpty message. In the future if we wanted to store some other kind of
  // value here, we can parse these empty values as with some other protocol
  // buffer (and the parser will see all default values).
  std::string empty_buffer;

  for (const DocumentKey& key : keys) {
    db_->current_transaction()->Put(
        LevelDbTargetDocumentKey::Key(target_id, key), empty_buffer);
    db_->current_transaction()->Put(
        LevelDbDocumentTargetKey::Key(key, target_id), empty_buffer);
    db_->reference_delegate()->AddReference(key);
  }
}

void LevelDbQueryCache::RemoveMatchingKeys(const DocumentKeySet& keys,
                                           TargetId target_id) {
  for (const DocumentKey& key : keys) {
    db_->current_transaction()->Delete(
        LevelDbTargetDocumentKey::Key(target_id, key));
    db_->current_transaction()->Delete(
        LevelDbDocumentTargetKey::Key(key, target_id));
    db_->reference_delegate()->RemoveReference(key);
  }
}

void LevelDbQueryCache::RemoveAllKeysForTarget(TargetId target_id) {
  std::string index_prefix = LevelDbTargetDocumentKey::KeyPrefix(target_id);
  auto index_iterator = db_->current_transaction()->NewIterator();
  index_iterator->Seek(index_prefix);

  LevelDbTargetDocumentKey row_key;
  for (; index_iterator->Valid(); index_iterator->Next()) {
    absl::string_view index_key = index_iterator->key();

    // Only consider rows matching this specific target_id.
    if (!row_key.Decode(index_key) || row_key.target_id() != target_id) {
      break;
    }
    const DocumentKey& document_key = row_key.document_key();

    // Delete both index rows
    db_->current_transaction()->Delete(index_key);
    db_->current_transaction()->Delete(
        LevelDbDocumentTargetKey::Key(document_key, target_id));
  }
}

DocumentKeySet LevelDbQueryCache::GetMatchingKeys(TargetId target_id) {
  std::string index_prefix = LevelDbTargetDocumentKey::KeyPrefix(target_id);
  auto index_iterator = db_->current_transaction()->NewIterator();
  index_iterator->Seek(index_prefix);

  DocumentKeySet result;
  LevelDbTargetDocumentKey row_key;
  for (; index_iterator->Valid(); index_iterator->Next()) {
    // TODO(gsoltis): could we use a StartsWith instead?
    // Only consider rows matching this specific target_id.
    if (!row_key.Decode(index_iterator->key()) ||
        row_key.target_id() != target_id) {
      break;
    }

    result = result.insert(row_key.document_key());
  }

  return result;
}

bool LevelDbQueryCache::Contains(const DocumentKey& key) {
  // ignore sentinel rows when determining if a key belongs to a target.
  // Sentinel row just says the document exists, not that it's a member of any
  // particular target.
  std::string index_prefix = LevelDbDocumentTargetKey::KeyPrefix(key.path());
  auto index_iterator = db_->current_transaction()->NewIterator();
  index_iterator->Seek(index_prefix);

  for (; index_iterator->Valid() &&
         absl::StartsWith(index_iterator->key(), index_prefix);
       index_iterator->Next()) {
    LevelDbDocumentTargetKey row_key;
    if (row_key.Decode(index_iterator->key()) && !row_key.IsSentinel() &&
        row_key.document_key() == key) {
      return true;
    }
  }

  return false;
}

const SnapshotVersion& LevelDbQueryCache::GetLastRemoteSnapshotVersion() const {
  return last_remote_snapshot_version_;
}

void LevelDbQueryCache::SetLastRemoteSnapshotVersion(SnapshotVersion version) {
  last_remote_snapshot_version_ = std::move(version);
  metadata_->last_remote_snapshot_version =
      serializer_->EncodeVersion(last_remote_snapshot_version_);
  SaveMetadata();
}

void LevelDbQueryCache::EnumerateOrphanedDocuments(
    const OrphanedDocumentCallback& callback) {
  std::string document_target_prefix = LevelDbDocumentTargetKey::KeyPrefix();
  auto it = db_->current_transaction()->NewIterator();
  it->Seek(document_target_prefix);
  ListenSequenceNumber next_to_report = 0;
  DocumentKey key_to_report;
  LevelDbDocumentTargetKey key;

  for (; it->Valid() && absl::StartsWith(it->key(), document_target_prefix);
       it->Next()) {
    HARD_ASSERT(key.Decode(it->key()), "Failed to decode DocumentTarget key");
    if (key.IsSentinel()) {
      // if next_to_report is non-zero, report it, this is a new key so the last
      // one must be not be a member of any targets.
      if (next_to_report != 0) {
        callback(key_to_report, next_to_report);
      }
      // set next_to_report to be this sequence number. It's the next one we
      // might report, if we don't find any targets for this document.
      next_to_report =
          LevelDbDocumentTargetKey::DecodeSentinelValue(it->value());
      key_to_report = key.document_key();
    } else {
      // set next_to_report to be 0, we know we don't need to report this one
      // since we found a target for it.
      next_to_report = 0;
    }
  }
  // if next_to_report is non-zero, report it. We didn't find any targets for
  // that document, and we weren't asked to stop.
  if (next_to_report != 0) {
    callback(key_to_report, next_to_report);
  }
}

void LevelDbQueryCache::Save(const QueryData& query_data) {
  TargetId target_id = query_data.target_id();
  std::string key = LevelDbTargetKey::Key(target_id);
  db_->current_transaction()->Put(key,
                                  serializer_->EncodeQueryData(query_data));
}

bool LevelDbQueryCache::UpdateMetadata(const QueryData& query_data) {
  bool updated = false;
  if (query_data.target_id() > metadata_->highest_target_id) {
    metadata_->highest_target_id = query_data.target_id();
    updated = true;
  }

  if (query_data.sequence_number() >
      metadata_->highest_listen_sequence_number) {
    metadata_->highest_listen_sequence_number = query_data.sequence_number();
    updated = true;
  }

  return updated;
}

void LevelDbQueryCache::SaveMetadata() {
  db_->current_transaction()->Put(LevelDbTargetGlobalKey::Key(), metadata_);
}

QueryData LevelDbQueryCache::DecodeTarget(absl::string_view encoded) {
  StringReader reader{encoded};
  auto message = Message<firestore_client_Target>::TryParse(&reader);
  auto result = serializer_->DecodeQueryData(&reader, *message);
  if (!reader.ok()) {
    HARD_FAIL("Target proto failed to parse: %s", reader.status().ToString());
  }

  return result;
}

}  // namespace local
}  // namespace firestore
}  // namespace firebase
