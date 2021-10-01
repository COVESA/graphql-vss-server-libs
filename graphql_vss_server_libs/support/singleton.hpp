#pragma once

#include <atomic>
#include <deque>
#include <future>
#include <set>
#include <unordered_map>

#include "debug.hpp"
#include "demangle.hpp"
#include "spinlock.hpp"

#include "graphql_vss_server_libs-support_export.h"

class SingletonStorage;

class GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT BaseSingleton
{
private:
	SingletonStorage* m_storage;
	std::atomic<size_t> m_refCount;
	SpinLock m_spinlock;

public:
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT BaseSingleton(SingletonStorage* storage);
	virtual ~BaseSingleton();

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT BaseSingleton* ref();
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void unref();
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT size_t refCount() const;
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void detach();

	class Ref
	{
	protected:
		BaseSingleton* m_singleton;

	public:
		Ref()
			: m_singleton(nullptr)
		{
		}

		Ref(BaseSingleton* singleton)
			: m_singleton(singleton->ref())
		{
		}

		~Ref()
		{
			if (m_singleton)
				m_singleton->unref();
		}

		Ref(Ref const& other)
			: m_singleton(other.m_singleton->ref())
		{
		}

		Ref(Ref&& other)
			: m_singleton(other.m_singleton)
		{
			other.m_singleton = nullptr;
		}

		inline size_t refCount() const
		{
			return m_singleton->refCount();
		}
	};

#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT virtual const std::string_view getDisplayName() const;
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT virtual std::string toString() const;
#endif
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT friend std::ostream&
	operator<<(std::ostream& out, const BaseSingleton& singleton);

#if defined(GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT)
// split binaries may have different pointers for the same demangle<ResultValue).data()
// then keep the actual string_view so the hash works
#define KEY_STRING_VIEW 1
#endif

#if KEY_STRING_VIEW
	using Key = std::string_view;
#else
	using Key = uintptr_t;
#endif
};

template <typename ResultValue>
class Singleton final : public BaseSingleton
{
private:
	std::shared_future<std::shared_ptr<ResultValue>> m_future;

public:
	static inline Key getKey()
	{
#if KEY_STRING_VIEW
		return demangle<ResultValue>();
#else
		return reinterpret_cast<uintptr_t>(demangle<ResultValue>().data());
#endif
	}

	Singleton(SingletonStorage* storage)
		: BaseSingleton(storage)
		, m_future(ResultValue::createFuture(storage))
	{
		dbg(COLOR_CYAN << "Singleton created " << *this);
	}

	~Singleton()
	{
		dbg(COLOR_MAGENTA << "Singleton destroyed " << *this);
	}

	Singleton(Singleton const& other) = delete;
	Singleton(Singleton&& other) = delete;

	inline std::shared_ptr<ResultValue> value() const
	{
		dbg("Singleton wait value... " << *this);

#ifdef GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
		auto startTime = std::chrono::high_resolution_clock::now();
#endif

		auto v = m_future.get();

#ifdef GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
		auto endTime = std::chrono::high_resolution_clock::now();
		auto elapsed =
			std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		if (elapsed > 1)
			dbg(COLOR_BG_RED << "Singleton value " << *this
							 << " took too long [stats: elapsed=" << elapsed << "ms]");
#endif

		dbg("Singleton value " << *this << ": " << v);
		return v;
	}

#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
	const std::string_view getDisplayName() const override
	{
		return demangle<ResultValue>();
	}
#endif

	class Ref : public BaseSingleton::Ref
	{
	public:
		Ref(Singleton<ResultValue>* singleton)
			: BaseSingleton::Ref(dynamic_cast<BaseSingleton*>(singleton))
		{
		}

		Ref(Ref const& other)
			: BaseSingleton::Ref(other.m_singleton)
		{
		}

		Ref(Ref&& other)
		{
			m_singleton = other.m_singleton;
			other.m_singleton = nullptr;
		}

		Ref(BaseSingleton::Ref const& other)
			: BaseSingleton::Ref(other)
		{
		}

		Ref(BaseSingleton::Ref&& other)
			: BaseSingleton::Ref(std::move(other))
		{
		}

		inline BaseSingleton::Ref base() const
		{
			return BaseSingleton::Ref(m_singleton);
		}

		inline std::shared_ptr<ResultValue> value() const
		{
			return dynamic_cast<Singleton<ResultValue>*>(m_singleton)->value();
		}
	};
};

class SingletonStorage
{
public:
	template <typename ResultValue>
	typename Singleton<ResultValue>::Ref get()
	{
		auto key = Singleton<ResultValue>::getKey();

		Singleton<ResultValue>* ptr;

		std::unique_lock childrenLock(m_childrenLock);
		const auto& itr = m_children.find(key);
		if (itr != m_children.end())
		{
			ptr = dynamic_cast<Singleton<ResultValue>*>(itr->second);
			recycle(ptr);
		}
		else
		{
			ptr = new Singleton<ResultValue>(this);
			m_children[key] = ptr;
		}
		childrenLock.unlock();

		dbg("SingletonStorage " << this << " get=" << key << " result=" << *ptr);
		return typename Singleton<ResultValue>::Ref(ptr);
	}

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT ~SingletonStorage();

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void dispose(BaseSingleton* singleton);
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void recycle(BaseSingleton* singleton);
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT size_t pendingGarbageCollect() const;
	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT size_t garbageCollect();

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT void clear();

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT std::string
	toString(const std::string_view& separator = "\n\t") const;

	GRAPHQL_VSS_SERVER_LIBS_SUPPORT_EXPORT friend std::ostream&
	operator<<(std::ostream& out, const SingletonStorage& storage);

private:
	std::unordered_map<BaseSingleton::Key, BaseSingleton*> m_children;
	std::mutex m_childrenLock;

	std::set<BaseSingleton*> m_disposed;
	std::mutex m_disposedLock;

	std::deque<BaseSingleton::Key> moveDisposedToDeleteKeysUnlocked();
	size_t garbageCollectInternalUnlocked(std::deque<BaseSingleton::Key>&& deleteKeys);
	size_t garbageCollectUnlocked();
};
