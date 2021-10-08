// Copyright (C) 2021, Bayerische Motoren Werke Aktiengesellschaft (BMW AG),
//   Author: Alexander Domin (Alexander.Domin@bmw.de)
// Copyright (C) 2021, ProFUSION Sistemas e Soluções LTDA,
//   Author: Gustavo Sverzut Barbieri (barbieri@profusion.mobi)
//
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the
// Mozilla Public License, v. 2.0. If a copy of the MPL was
// not distributed with this file, You can obtain one at
// http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <thread>
#include <chrono>
#include <array>
#include <cstring>
#include <gtest/gtest.h>

#include <graphql_vss_server_libs/support/singleton.hpp>


#define TEST_STR "hello world"
#define TEST_NUMBER 42
#define CREATE_SLEEP_DURATION std::chrono::seconds(2)

static std::atomic<int> _live_instances = 0;
static SingletonStorage rootSingleton;

// Make sure complex types are properly created and destroyed
class SomeType
{
public:
	static std::shared_ptr<SomeType> create()
	{
		// force something on the stack, so it's not placed in ELF's data segment and force
		// the memory to vanish once the function returns.
		char on_stack[sizeof(TEST_STR)];

		dbg(COLOR_BG_RED ">>>> create SomeType: SLEEPING "
			<< std::chrono::duration_cast<std::chrono::milliseconds>(CREATE_SLEEP_DURATION).count()
			<< "ms...");
		std::this_thread::sleep_for(CREATE_SLEEP_DURATION);
		dbg(COLOR_BG_RED ">>>> create SomeType: RETURNING");

		memcpy(on_stack, TEST_STR, sizeof(on_stack));

		return std::make_shared<SomeType>(on_stack);
	}

	static std::future<std::shared_ptr<SomeType>> createFuture(SingletonStorage*)
	{
		return std::async(std::launch::async, create);
	}

	SomeType(const char* str)
		: m_str(str)
	{
		dbg(COLOR_BG_BLUE "SomeType created " << this << " str: " << m_str);
		_live_instances++;
	}

	SomeType(SomeType const& other) = delete; // avoid copies, it's an usage bug!
	SomeType(SomeType&& other) = delete;	  // avoid references, it's an usage bug!

	~SomeType()
	{
		dbg(COLOR_BG_BLUE "~SomeType destroyed " << this << " str: " << m_str);
		_live_instances--;
	}

	const std::string str() const
	{
		return this->m_str;
	}

private:
	std::string m_str;
};

struct ScalarResult
{
	// the struct size should be the sizeof its single member (see assert below)
	short i;

	static std::shared_ptr<ScalarResult> create()
	{
		dbg(COLOR_BG_RED ">>>> create ScalarResult: SLEEPING "
			<< std::chrono::duration_cast<std::chrono::milliseconds>(CREATE_SLEEP_DURATION).count()
			<< "ms...");
		std::this_thread::sleep_for(CREATE_SLEEP_DURATION);
		dbg(COLOR_BG_RED ">>>> create ScalarResult: RETURNING");

		return std::make_shared<ScalarResult>();
	}

	static std::future<std::shared_ptr<ScalarResult>> createFuture(SingletonStorage*)
	{
		return std::async(std::launch::async, create);
	}

	ScalarResult()
		: i(TEST_NUMBER)
	{
		dbg(COLOR_BG_BLUE "ScalarResult created " << this << " i: " << i);
		_live_instances++;
	}

	ScalarResult(ScalarResult const& other) = delete; // avoid copies, it's an usage bug!
	ScalarResult(ScalarResult&& other) = delete;	  // avoid references, it's an usage bug!

	~ScalarResult()
	{
		dbg(COLOR_BG_BLUE "~ScalarResult destroyed " << this << " i: " << i);
		_live_instances--;
	}
};

// Make sure types dependent on another are properly handled
class DependOnSomeType
{
public:
	static std::shared_ptr<DependOnSomeType> create(SingletonStorage* storage)
	{
		return std::make_shared<DependOnSomeType>(storage->get<SomeType>());
	}

	static std::future<std::shared_ptr<DependOnSomeType>> createFuture(SingletonStorage* storage)
	{
		return std::async(std::launch::async, create, storage);
	}

	DependOnSomeType(Singleton<SomeType>::Ref&& someType)
		: m_someType(std::move(someType))
	{
		dbg(COLOR_BG_CYAN "DependOnSomeType created " << this << " someType: " << &m_someType);
		_live_instances++;
	}

	DependOnSomeType(DependOnSomeType const& other) = delete; // avoid copies, it's an usage bug!
	DependOnSomeType(DependOnSomeType&& other) = delete;	  // avoid references, it's an usage bug!

	~DependOnSomeType()
	{
		dbg(COLOR_BG_CYAN "~DependOnSomeType destroyed " << this << " someType: " << &m_someType);
		_live_instances--;
	}

	const std::string someTypeStr() const
	{
		return this->m_someType.value()->str();
	}

private:
	Singleton<SomeType>::Ref m_someType;
};

TEST(singleton_test, sequential_keeps_string)
{
	dbg(COLOR_BG_CYAN "# STARTING: SEQUENTIAL TESTS");

	do
	{ // new scope to force result1, result2 and others to be deleted
		std::future<std::shared_ptr<SomeType>> test1 =
			std::async(std::launch::async, []() -> std::shared_ptr<SomeType> {
				return rootSingleton.get<SomeType>().value();
			});
		dbg(COLOR_BG_CYAN "#### GOT TEST1 FUTURE: " << &test1 << ", now wait...");

		std::shared_ptr<SomeType> result1 = test1.get();
		dbg(COLOR_BG_CYAN "#### GOT SINGLETON FOR TEST1: " << result1 << " " << result1->str());

		std::shared_ptr<SomeType> result2 =
			std::async(std::launch::async, []() -> std::shared_ptr<SomeType> {
				return rootSingleton.get<SomeType>().value();
			}).get();
		dbg(COLOR_BG_CYAN "#### GOT SINGLETON FOR TEST2: " << result2 << " " << result2->str());

		EXPECT_EQ(result1->str(), TEST_STR);
		EXPECT_EQ(result2->str(), TEST_STR);
		EXPECT_EQ(result1, result2);
		EXPECT_EQ(_live_instances, 1);

		dbg(COLOR_BG_CYAN "#### Singletons: " << rootSingleton.toString());

		rootSingleton.clear();

		// must be still 1 since result1/result2 keep reference
		ASSERT_EQ(_live_instances, 1);
	} while (0);

	assert(_live_instances == 0);
}

// same as sequentialTests(), but with a Scalar (short)
// not expected to be used, but make sure that works as well
TEST(singleton_test, singleton_keep_same_scalar)
{
	dbg(COLOR_BG_CYAN "# STARTING: SCALAR TESTS -- sizeof(ScalarResult) "
		<< sizeof(ScalarResult) << ", shared_ptr: " << sizeof(std::shared_ptr<ScalarResult>));

	assert(sizeof(ScalarResult) == sizeof(short));

	do
	{ // new scope to force result1, result2 and others to be deleted
		std::future<std::shared_ptr<ScalarResult>> test1 =
			std::async(std::launch::async, []() -> std::shared_ptr<ScalarResult> {
				return rootSingleton.get<ScalarResult>().value();
			});
		dbg(COLOR_BG_CYAN "#### GOT TEST1 FUTURE: " << &test1 << ", now wait...");

		std::shared_ptr<ScalarResult> result1 = test1.get();
		dbg(COLOR_BG_CYAN "#### GOT SINGLETON FOR TEST1: " << result1 << " " << result1->i);

		std::shared_ptr<ScalarResult> result2 =
			std::async(std::launch::async, []() -> std::shared_ptr<ScalarResult> {
				return rootSingleton.get<ScalarResult>().value();
			}).get();
		dbg(COLOR_BG_CYAN "#### GOT SINGLETON FOR TEST2: " << result2 << " " << result2->i);

		EXPECT_EQ(result1->i, TEST_NUMBER);
		EXPECT_EQ(result2->i, TEST_NUMBER);
		EXPECT_EQ(result1, result2);
		EXPECT_EQ(_live_instances, 1);

		dbg(COLOR_BG_CYAN "#### Singletons: " << rootSingleton);

		rootSingleton.clear();

		// must be still 1 since result1/result2 keep reference
		EXPECT_EQ(_live_instances, 1);
	} while (0);

	EXPECT_EQ(_live_instances, 0);
}

TEST(singleton_test, parallel_test)
{
	dbg(COLOR_BG_MAGENTA "# STARTING: PARALLEL TESTS");

	do
	{ // new scope to force result1, result2, result3 and others to be deleted
		std::future<std::shared_ptr<SomeType>> test1 =
			std::async(std::launch::async, []() -> std::shared_ptr<SomeType> {
				dbg(COLOR_BG_MAGENTA "TEST 1");
				std::shared_ptr<SomeType> result =
					rootSingleton.get<SomeType>().value();
				dbg(COLOR_BG_MAGENTA "TEST 1 done, result = " << result << " " << result->str());
				return result;
			});

		std::future<std::shared_ptr<SomeType>> test2 =
			std::async(std::launch::async, []() -> std::shared_ptr<SomeType> {
				dbg(COLOR_BG_MAGENTA "TEST 2");
				std::shared_ptr<SomeType> result =
					rootSingleton.get<SomeType>().value();
				dbg(COLOR_BG_MAGENTA "TEST 2 done, result = " << result << " " << result->str());
				return result;
			});

		dbg(COLOR_BG_MAGENTA "# WAIT TEST1");
		std::shared_ptr<SomeType> result1 = test1.get();

		dbg(COLOR_BG_MAGENTA "# WAIT TEST2");
		std::shared_ptr<SomeType> result2 = test2.get();

		dbg(COLOR_BG_MAGENTA "# TEST3: Get after both tests are done");
		std::shared_ptr<SomeType> result3 =
			rootSingleton.get<SomeType>().value();

		dbg(COLOR_BG_MAGENTA "# TEST4: Get sibling (other path)");
		std::shared_ptr<ScalarResult> result4 =
			rootSingleton.get<ScalarResult>().value();

		dbg(COLOR_BG_MAGENTA "#### GOT SINGLETON FOR TEST1: " << result1 << " " << result1->str());
		dbg(COLOR_BG_MAGENTA "#### GOT SINGLETON FOR TEST2: " << result2 << " " << result2->str());
		dbg(COLOR_BG_MAGENTA "#### GOT SINGLETON FOR TEST3: " << result3 << " " << result3->str());
		dbg(COLOR_BG_MAGENTA "#### GOT SINGLETON FOR TEST4: " << result4 << " " << result4->i);

		EXPECT_EQ(result1->str(), TEST_STR);
		EXPECT_EQ(result2->str(), TEST_STR);
		EXPECT_EQ(result3->str(), TEST_STR);
		EXPECT_EQ(result4->i, TEST_NUMBER);
		EXPECT_EQ(result1, result2);
		EXPECT_EQ(result1, result3);
		EXPECT_NE((void*)result1.get(), (void*)result4.get());
		EXPECT_EQ(_live_instances, 2);

		dbg(COLOR_BG_MAGENTA "#### Singletons: " << rootSingleton);

		rootSingleton.clear();

		// must be still 2 since result1/result2/result3 keep reference
		EXPECT_EQ(_live_instances, 2);
	} while (0);

	EXPECT_EQ(_live_instances, 0) << "there are _live_instances";
}

TEST(singleton_test, ref)
{
	dbg(COLOR_BG_MAGENTA "# STARTING: REFERENCE TESTS");

	do
	{ // new scope to force references to be gone
		auto r1 = rootSingleton.get<SomeType>();
		EXPECT_EQ(r1.refCount(), 1);

		auto r2 = rootSingleton.get<SomeType>();
		EXPECT_EQ(r2.refCount(), 2);

		auto r3 = r2;
		EXPECT_EQ(r3.refCount(), 3);
		EXPECT_EQ(r1.refCount(), 3); // the same ptr, so must match

		do
		{
			auto r4 = r2;
			EXPECT_EQ(r3.refCount(), 4);
		}
		while (0);
		EXPECT_EQ(r3.refCount(), 3);

		auto r4 = std::move(r3);
		EXPECT_EQ(r1.refCount(), 3);

		dbg(COLOR_BG_MAGENTA "#### Singletons: " << rootSingleton);

		// wait value to be created, otherwise instance may not be created yet
		auto result1 = r1.value();
		EXPECT_EQ(result1->str(), TEST_STR);

		EXPECT_EQ(_live_instances, 1);
	} while (0);

	// force clear with references alive
	rootSingleton.clear();

	EXPECT_EQ(_live_instances, 0) << "there are _live_instances";
}

TEST(singleton_test, ref_detach)
{
	dbg(COLOR_BG_MAGENTA "# STARTING: REFERENCE DETACH TESTS");

	do
	{ // new scope to force references to be gone
		auto r1 = rootSingleton.get<SomeType>();
		EXPECT_EQ(r1.refCount(), 1);

		auto r2 = rootSingleton.get<SomeType>();
		EXPECT_EQ(r2.refCount(), 2);

		auto r3 = r2;
		EXPECT_EQ(r3.refCount(), 3);
		EXPECT_EQ(r1.refCount(), 3); // the same ptr, so must match

		do
		{
			auto r4 = r2;
			EXPECT_EQ(r3.refCount(), 4);
		}
		while (0);
		EXPECT_EQ(r3.refCount(), 3);

		auto r4 = std::move(r3);
		EXPECT_EQ(r1.refCount(), 3);

		dbg(COLOR_BG_MAGENTA "#### Singletons: " << rootSingleton);

		// force clear with references alive
		rootSingleton.clear();

		// wait value to be created, otherwise instance may not be created yet
		auto result1 = r1.value();
		EXPECT_EQ(result1->str(), TEST_STR);

		// must be still 1 since they keep reference
		EXPECT_EQ(_live_instances, 1);
	} while (0);

	EXPECT_EQ(_live_instances, 0) << "there are _live_instances";
}

TEST(singleton_test, recycle_disposed)
{
	dbg(COLOR_BG_MAGENTA "# STARTING: RECYCLE DISPOSED TESTS");

	SomeType *p1;
	do
	{
		auto r1 = rootSingleton.get<SomeType>();
		EXPECT_EQ(r1.refCount(), 1);

		auto result1 = r1.value();
		EXPECT_EQ(result1->str(), TEST_STR);
		p1 = result1.get();
	} while (0);

	EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 1);

	do
	{
		auto r2 = rootSingleton.get<SomeType>();
		EXPECT_EQ(r2.refCount(), 1);

		auto result2 = r2.value();
		EXPECT_EQ(result2->str(), TEST_STR);
		auto p2 = result2.get();
		EXPECT_EQ(p1, p2);

		EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 0);
	} while (0);

	EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 1);
	rootSingleton.garbageCollect();

	EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 0);

	do
	{
		auto r3 = rootSingleton.get<SomeType>();
		EXPECT_EQ(r3.refCount(), 1);

		auto result3 = r3.value();
		EXPECT_EQ(result3->str(), TEST_STR);
		auto p3 = result3.get();
		EXPECT_NE(p1, p3);

		EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 0);
	} while (0);

	EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 1);

	rootSingleton.clear();

	EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 0);

	EXPECT_EQ(_live_instances, 0) << "there are _live_instances";
}

TEST(singleton_test, dependent_type)
{
	dbg(COLOR_BG_MAGENTA "# STARTING: DEPENDENT TYPE TESTS");

	do
	{
		auto r1 = rootSingleton.get<DependOnSomeType>();
		auto result1 = r1.value();
		EXPECT_EQ(result1->someTypeStr(), TEST_STR);

		EXPECT_EQ(_live_instances, 2) << "there are _live_instances";
	} while (0);

	EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 1);

	dbg(COLOR_BG_MAGENTA "#### Singletons: " << rootSingleton);
	rootSingleton.clear();

	EXPECT_EQ(rootSingleton.pendingGarbageCollect(), 0);

	EXPECT_EQ(_live_instances, 0) << "there are _live_instances";
}

int main(int argc, char **argv)
{
	std::cerr << std::boolalpha;
	::testing::InitGoogleTest(&argc, argv);
	dbg("Test Singletons (main thread)");
	return RUN_ALL_TESTS();
}
