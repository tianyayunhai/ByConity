/*
 * Copyright 2016-2023 ClickHouse, Inc.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/*
 * This file may have been modified by Bytedance Ltd. and/or its affiliates (“ Bytedance's Modifications”).
 * All Bytedance's Modifications are Copyright (2023) Bytedance Ltd. and/or its affiliates.
 */

#include <Coordination/KeeperLogStore.h>
#include <IO/CompressionMethod.h>

namespace DB
{

KeeperLogStore::KeeperLogStore(const std::string & changelogs_path, uint64_t rotate_interval_, bool force_sync_, bool compress_logs_)
    : log(getLogger("KeeperLogStore"))
    , changelog(changelogs_path, rotate_interval_, force_sync_, log, compress_logs_)
{
    if (force_sync_)
        LOG_INFO(log, "force_sync enabled");
    else
        LOG_INFO(log, "force_sync disabled");
}

uint64_t KeeperLogStore::start_index() const
{
    std::lock_guard lock(changelog_lock);
    return changelog.getStartIndex();
}

void KeeperLogStore::init(uint64_t last_commited_log_index, uint64_t logs_to_keep)
{
    std::lock_guard lock(changelog_lock);
    changelog.readChangelogAndInitWriter(last_commited_log_index, logs_to_keep);
}

uint64_t KeeperLogStore::next_slot() const
{
    std::lock_guard lock(changelog_lock);
    return changelog.getNextEntryIndex();
}

nuraft::ptr<nuraft::log_entry> KeeperLogStore::last_entry() const
{
    std::lock_guard lock(changelog_lock);
    return changelog.getLastEntry();
}

uint64_t KeeperLogStore::append(nuraft::ptr<nuraft::log_entry> & entry)
{
    std::lock_guard lock(changelog_lock);
    uint64_t idx = changelog.getNextEntryIndex();
    changelog.appendEntry(idx, entry);
    return idx;
}


void KeeperLogStore::write_at(uint64_t index, nuraft::ptr<nuraft::log_entry> & entry)
{
    std::lock_guard lock(changelog_lock);
    changelog.writeAt(index, entry);
}

nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> KeeperLogStore::log_entries(uint64_t start, uint64_t end)
{
    std::lock_guard lock(changelog_lock);
    return changelog.getLogEntriesBetween(start, end);
}

nuraft::ptr<nuraft::log_entry> KeeperLogStore::entry_at(uint64_t index)
{
    std::lock_guard lock(changelog_lock);
    return changelog.entryAt(index);
}

uint64_t KeeperLogStore::term_at(uint64_t index)
{
    std::lock_guard lock(changelog_lock);
    auto entry = changelog.entryAt(index);
    if (entry)
        return entry->get_term();
    return 0;
}

nuraft::ptr<nuraft::buffer> KeeperLogStore::pack(uint64_t index, int32_t cnt)
{
    std::lock_guard lock(changelog_lock);
    return changelog.serializeEntriesToBuffer(index, cnt);
}

bool KeeperLogStore::compact(uint64_t last_log_index)
{
    std::lock_guard lock(changelog_lock);
    changelog.compact(last_log_index);
    return true;
}

bool KeeperLogStore::flush()
{
    std::lock_guard lock(changelog_lock);
    changelog.flush();
    return true;
}

void KeeperLogStore::apply_pack(uint64_t index, nuraft::buffer & pack)
{
    std::lock_guard lock(changelog_lock);
    changelog.applyEntriesFromBuffer(index, pack);
}

uint64_t KeeperLogStore::size() const
{
    std::lock_guard lock(changelog_lock);
    return changelog.size();
}

void KeeperLogStore::end_of_append_batch(uint64_t /*start_index*/, uint64_t /*count*/)
{
    std::lock_guard lock(changelog_lock);
    changelog.flush();
}

nuraft::ptr<nuraft::log_entry> KeeperLogStore::getLatestConfigChange() const
{
    std::lock_guard lock(changelog_lock);
    return changelog.getLatestConfigChange();
}

}
