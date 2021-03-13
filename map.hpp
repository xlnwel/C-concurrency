#ifndef CONCURRENCY_THREADSAFE_MAP_H_
#define CONCURRENCY_THREADSAFE_MAP_H_

#include <vector>
#include <utility>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include "list.hpp"

namespace utility{
    template<typename Key, typename Value, typename Hash=std::hash<Key>>
    class LockBasedMap {
        class Bucket {
            using KVPair = std::pair<Key, Value>;
            using DataType = LockBasedList<KVPair>;
        public:
            Value at(const Key& k, const Value& v= {}) const {
                auto p = data.find_if([&k](const KVPair& d) {return d.first == k;});
                return p?p->second: v;
            }
            void insert_or_assign(const Key& k, Value&& v) {
                data.remove_if([&k](const KVPair& d) { return d.first == k;});
                data.push_front({k, std::forward<Value>(v)});
            }
            void erase(const Key& k) {
                data.remove_if([&k](const KVPair& d) { return d.first == k;});
            }
        private:
            // no mutex for bucket as the list is thread safe
            DataType data;
        };
    public:
        Value at(const Key& k, const Value& v= {}) {
            return get_bucket(k).at(k, v);
        }
        void insert_or_assign(const Key& k, Value&& v) {
            return get_bucket(k).insert_or_assign(k, std::forward<Value>(v));
        }
        void erase(const Key& k) {
            return get_buecket(k).erase(k);
        }
    private:
        Bucket& get_bucket(const Key& k) const {
            const std::size_t idx = hasher(k) % buckets.size();
            return buckets[idx];
        }
        std::vector<Bucket> buckets;
        Hash hasher;
    };


}

#endif