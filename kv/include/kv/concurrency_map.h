// MIT License

// Copyright (c) 2021 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <map>
#include <vector>
#include <string>
#include <iterator>
#include <algorithm>

template<typename K, typename V, typename Hash = std::hash<K>>
class ConcurrentHashMap
{
public:
	ConcurrentHashMap(unsigned bucketNumber = kDefaultBucketNum, const Hash& hash = Hash())
		: table_(bucketNumber),
		hash_(hash)
	{
	}

	template<typename Predicate>
	bool for_one(const K& key, Predicate& p)
	{
		return table_[hashcode(key)].for_one(key, p);
	}

	template<typename Predicate>
	void for_each(Predicate& p)
	{
		for (auto& bucket : table_)
		{
			bucket.for_each(p);
		}
	}
	
	//Insert the map elemenet, but won't cover
	void insert(const K& key, V&& value)
	{
		table_[hashcode(key)].insert(key, std::move(value));
	}

	//Insert the map elemenet, but will cover
	void put(const K& key, V&& value)
	{
		table_[hashcode(key)].put(key, std::move(value));
	}

	//Get the element value by the key
	V get(const K& key)
	{
		if (table_[hashcode(key)].get(key) == NULL){
			std::cout<< "Key does not exist!"<<std::endl;
			exit(0);
		}
		else
			return table_[hashcode(key)].get(key);
	}

	//Erase the element by the key
	void erase(const K& key)
	{
		table_[hashcode(key)].erase(key);
	}

	//Return the whole element size of map
	std::size_t size() const
	{
		std::size_t size = 0;
		for (auto& bucket : table_)
		{
			size += bucket.size();
		}
		return size;
	}

	//Return the element number. If the element exists, return 1. If not, return 0
	std::size_t count(const K& key)
	{
		return table_[hashcode(key)].count(key);
	}

private:
	static const unsigned kDefaultBucketNum = 1013;  //Prime Number is better

	class Bucket
	{
	public:
		void insert(const K& key, V&& value)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			item_.emplace(key, std::move(value));
		}

		void put(const K& key, V&& value)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			item_.erase(key);
			item_.emplace(key, std::move(value));
		}

		V get(const K& key) {
			std::lock_guard<std::mutex> lock(mutex_);
			const ConstIterator it = item_.find(key);
			if (it == item_.end()) 
				return NULL;
			else
				return it->second;
		}

		void erase(const K& key)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			item_.erase(key);
		}

		template<typename Predicate>
		bool for_one(const K& key, Predicate& p)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			const ConstIterator it = item_.find(key);
			return it == item_.end() ? false : (p(it->second), true);
		}

		template<typename Predicate>
		void for_each(Predicate& p)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			std::for_each(item_.begin(), item_.end(), p);
		}

		std::size_t size() const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			return item_.size();
		}

		std::size_t count(const K& key)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			return item_.count(key);
		}

	private:
		using Item = std::map<K, V>;
		using ConstIterator = typename Item::const_iterator;
		Item item_;
		mutable std::mutex mutex_;
	};

	inline std::size_t hashcode(const K& key)
	{
		return hash_(key) % table_.size();
	};

	std::vector<Bucket> table_;
	Hash hash_;
};
