#include <assert.h>
#include <sstream>

#include "singleton.hpp"

BaseSingleton::BaseSingleton(SingletonStorage* storage)
	: m_storage(storage)
	, m_refCount(0)
	, m_spinlock(SpinLock(this))
{
}

BaseSingleton::~BaseSingleton()
{
	assert(m_refCount == 0);
}

BaseSingleton* BaseSingleton::ref()
{
	m_refCount++;
	return this;
}

void BaseSingleton::unref()
{
	if (m_refCount-- > 1)
		return;

	std::unique_lock lock(m_spinlock);
	if (m_storage)
		m_storage->dispose(this);
	else
	{
		lock.unlock();
		delete this;
	}
}

size_t BaseSingleton::refCount() const
{
	return m_refCount;
}

void BaseSingleton::detach()
{
	std::lock_guard guard(m_spinlock);
	m_storage = nullptr;
}

#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
const std::string_view BaseSingleton::getDisplayName() const
{
	return "BaseSingleton";
}

std::string BaseSingleton::toString() const
{
	std::ostringstream out;
	out << (void*)this << " (" << getDisplayName() << ") storage=" << m_storage
		<< " ref=" << m_refCount;
	return out.str();
}
#endif // GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG

std::ostream& operator<<(std::ostream& out, const BaseSingleton& singleton)
{
#if GRAPHQL_VSS_SERVER_LIBS_SUPPORT_DEBUG
	out << singleton.toString();
#else
	out << &singleton;
#endif
	return out;
}

SingletonStorage::~SingletonStorage()
{
	dbg("~SingletonStorage " << this << " children=" << m_children.size());
	clear();
}

void SingletonStorage::dispose(BaseSingleton* singleton)
{
	dbg("SingletonStorage " << this << " dispose=" << *singleton);
	std::unique_lock disposedLock(m_disposedLock);
	m_disposed.insert(singleton);
}

void SingletonStorage::recycle(BaseSingleton* singleton)
{
	std::unique_lock disposedLock(m_disposedLock);
	auto itr = m_disposed.find(singleton);
	if (itr == m_disposed.end())
		return;

	dbg("SingletonStorage " << this << " recycle=" << *singleton);
	m_disposed.erase(itr);
}

size_t SingletonStorage::pendingGarbageCollect() const
{
	return m_disposed.size();
}

size_t SingletonStorage::garbageCollect()
{
	std::unique_lock childrenLock(m_childrenLock);
	return garbageCollectUnlocked();
}

std::deque<BaseSingleton::Key> SingletonStorage::moveDisposedToDeleteKeysUnlocked()
{
	std::unique_lock disposedLock(m_disposedLock);
	size_t toBeDisposed = m_disposed.size();
	std::deque<BaseSingleton::Key> deletedKeys;
	if (toBeDisposed == 0)
		return deletedKeys;

	for (auto& itr : m_children)
	{
		if (m_disposed.find(itr.second) != m_disposed.end())
		{
			dbg("SingletonStorage " << this << " gc will collect=" << *itr.second);
			assert(itr.second->refCount() == 0);
			deletedKeys.push_back(itr.first);
			if (deletedKeys.size() == toBeDisposed)
				break;
		}
	}

	size_t disposedCount = deletedKeys.size();
	dbg("SingletonStorage " << this << " gc=" << disposedCount << " of " << toBeDisposed);
	assert(disposedCount == toBeDisposed);
	m_disposed.clear();

	return deletedKeys;
}

size_t
SingletonStorage::garbageCollectInternalUnlocked(std::deque<BaseSingleton::Key>&& deletedKeys)
{
	for (auto& key : deletedKeys)
	{
		auto itr = m_children.find(key);
		assert(itr != m_children.end());
		dbg("SingletonStorage " << this << " gc singleton=" << *itr->second);
		delete itr->second;
		m_children.erase(itr);
	}

	return deletedKeys.size();
}

size_t SingletonStorage::garbageCollectUnlocked()
{
	size_t disposedCount = 0;

	while (true)
	{
		// dependent types will dispose when they are disposed, thus a loop
		size_t current = garbageCollectInternalUnlocked(moveDisposedToDeleteKeysUnlocked());
		if (current == 0)
			break;
		disposedCount++;
	}

	dbg("SingletonStorage " << this << " gc total=" << disposedCount);

	return disposedCount;
}

void SingletonStorage::clear()
{
	std::unique_lock childrenLock(m_childrenLock);
	garbageCollectUnlocked();

	for (auto& itr : m_children)
	{
		dbg("SingletonStorage " << this << " clear with alive singleton=" << *itr.second
								<< ": detached");
		itr.second->detach();
	}
	m_children.clear();
}

std::string SingletonStorage::toString(const std::string_view& separator) const
{
	std::ostringstream out;

	out << "{";
	bool isFirst = true;
	for (const auto& itr : m_children)
	{
		if (isFirst)
			isFirst = false;
		else
			out << separator;
		out << *itr.second;
	}
	out << "}";
	return out.str();
}

std::ostream& operator<<(std::ostream& out, const SingletonStorage& storage)
{
	out << storage.toString();
	return out;
}
