// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <CppUnitTest.h>

#include <CommonUtilities/cnr/FixedVector.h>

#include <array>
#include <ranges>
#include <stdexcept>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UtilityTests
{
	struct ValueType
	{
		int value = 0;
		ValueType() : value(7) {}
		explicit ValueType(int v) : value(v) {}
		bool operator==(const ValueType& other) const { return value == other.value; }
	};

	struct CountingType
	{
		static int Alive;
		static int Constructions;
		static int Destructions;

		int value = 0;
		CountingType() : value(0) { ++Alive; ++Constructions; }
		explicit CountingType(int v) : value(v) { ++Alive; ++Constructions; }
		CountingType(const CountingType& other) : value(other.value) { ++Alive; ++Constructions; }
		CountingType(CountingType&& other) noexcept : value(other.value) { other.value = -1; ++Alive; ++Constructions; }
		CountingType& operator=(const CountingType& other) { value = other.value; return *this; }
		CountingType& operator=(CountingType&& other) noexcept { value = other.value; other.value = -1; return *this; }
		~CountingType() { --Alive; ++Destructions; }

		static void Reset()
		{
			Alive = 0;
			Constructions = 0;
			Destructions = 0;
		}
	};

	int CountingType::Alive = 0;
	int CountingType::Constructions = 0;
	int CountingType::Destructions = 0;

	struct CounterRange
	{
		struct Iterator
		{
			using iterator_concept = std::input_iterator_tag;
			using iterator_category = std::input_iterator_tag;
			using value_type = int;
			using difference_type = std::ptrdiff_t;

			int value = 0;
			int finish = 0;

			int operator*() const { return value; }
			Iterator& operator++() { ++value; return *this; }
			void operator++(int) { ++value; }
			bool operator==(const Iterator& other) const { return value == other.value; }
			bool operator!=(const Iterator& other) const { return value != other.value; }
		};

		int start = 0;
		int finish = 0;

		Iterator begin() const { return Iterator{ start, finish }; }
		Iterator end() const { return Iterator{ finish, finish }; }
	};

	TEST_CLASS(TestFixedVector)
	{
	public:
		TEST_METHOD(DefaultState)
		{
			pmon::util::cnr::FixedVector<int, 4> vec;
			Assert::IsTrue(vec.Empty());
			Assert::AreEqual<size_t>(0u, vec.Size());
			Assert::AreEqual<size_t>(4u, vec.Capacity());
			Assert::AreEqual<size_t>(4u, vec.MaxSize());
			Assert::IsTrue(vec.begin() == vec.end());
			Assert::IsTrue(vec.CBegin() == vec.CEnd());
			Assert::IsTrue(vec.Data() != nullptr);
		}

		TEST_METHOD(CountConstructors)
		{
			pmon::util::cnr::FixedVector<ValueType, 4> vecDefault(3);
			Assert::AreEqual<size_t>(3u, vecDefault.Size());
			Assert::AreEqual(7, vecDefault[0].value);
			Assert::AreEqual(7, vecDefault[1].value);
			Assert::AreEqual(7, vecDefault[2].value);

			pmon::util::cnr::FixedVector<ValueType, 4> vecFill(2, ValueType{ 9 });
			Assert::AreEqual<size_t>(2u, vecFill.Size());
			Assert::AreEqual(9, vecFill[0].value);
			Assert::AreEqual(9, vecFill[1].value);
		}

		TEST_METHOD(InitializerListAndIteratorConstructors)
		{
			pmon::util::cnr::FixedVector<int, 5> initVec{ 1, 2, 3 };
			Assert::AreEqual<size_t>(3u, initVec.Size());
			Assert::AreEqual(1, initVec[0]);
			Assert::AreEqual(2, initVec[1]);
			Assert::AreEqual(3, initVec[2]);

			std::array<int, 3> source{ 4, 5, 6 };
			pmon::util::cnr::FixedVector<int, 5> iterVec(source.begin(), source.end());
			Assert::AreEqual<size_t>(3u, iterVec.Size());
			Assert::AreEqual(4, iterVec[0]);
			Assert::AreEqual(5, iterVec[1]);
			Assert::AreEqual(6, iterVec[2]);
		}

		TEST_METHOD(FromRangeConstructorAndAssign)
		{
			std::vector<int> source{ 7, 8, 9 };
			pmon::util::cnr::FixedVector<int, 5> rangeVec(std::from_range, source);
			Assert::AreEqual<size_t>(3u, rangeVec.Size());
			Assert::AreEqual(7, rangeVec[0]);
			Assert::AreEqual(8, rangeVec[1]);
			Assert::AreEqual(9, rangeVec[2]);

			pmon::util::cnr::FixedVector<int, 5> assigned;
			assigned.Assign(std::from_range, source);
			Assert::AreEqual<size_t>(3u, assigned.Size());
			Assert::AreEqual(7, assigned[0]);
			Assert::AreEqual(8, assigned[1]);
			Assert::AreEqual(9, assigned[2]);
		}

		TEST_METHOD(CopyAndMove)
		{
			pmon::util::cnr::FixedVector<int, 4> src{ 1, 2, 3 };
			pmon::util::cnr::FixedVector<int, 4> copy(src);
			Assert::AreEqual<size_t>(3u, copy.Size());
			Assert::AreEqual(1, copy[0]);
			Assert::AreEqual(2, copy[1]);
			Assert::AreEqual(3, copy[2]);

			pmon::util::cnr::FixedVector<int, 4> assigned;
			assigned = src;
			Assert::AreEqual<size_t>(3u, assigned.Size());
			Assert::AreEqual(1, assigned[0]);
			Assert::AreEqual(2, assigned[1]);
			Assert::AreEqual(3, assigned[2]);

			pmon::util::cnr::FixedVector<int, 4> moved(std::move(src));
			Assert::AreEqual<size_t>(3u, moved.Size());
			Assert::AreEqual(1, moved[0]);
			Assert::AreEqual(2, moved[1]);
			Assert::AreEqual(3, moved[2]);
			Assert::IsTrue(src.Empty());

			pmon::util::cnr::FixedVector<int, 4> moveAssign;
			moveAssign = std::move(moved);
			Assert::AreEqual<size_t>(3u, moveAssign.Size());
			Assert::AreEqual(1, moveAssign[0]);
			Assert::AreEqual(2, moveAssign[1]);
			Assert::AreEqual(3, moveAssign[2]);
			Assert::IsTrue(moved.Empty());
		}

		TEST_METHOD(PushPopEmplace)
		{
			pmon::util::cnr::FixedVector<ValueType, 4> vec;
			ValueType first{ 1 };
			vec.PushBack(first);
			vec.PushBack(ValueType{ 2 });
			auto& ref = vec.EmplaceBack(3);

			Assert::AreEqual<size_t>(3u, vec.Size());
			Assert::AreEqual(1, vec[0].value);
			Assert::AreEqual(2, vec[1].value);
			Assert::AreEqual(3, vec[2].value);
			Assert::IsTrue(&ref == &vec.Back());

			vec.PopBack();
			Assert::AreEqual<size_t>(2u, vec.Size());
			Assert::AreEqual(2, vec.Back().value);
		}

		TEST_METHOD(ResizeClearAndLifetime)
		{
			CountingType::Reset();
			{
				pmon::util::cnr::FixedVector<CountingType, 5> vec;
				vec.Resize(3);
				Assert::AreEqual(3, CountingType::Alive);
				vec.Resize(1);
				Assert::AreEqual(1, CountingType::Alive);
				vec.Clear();
				Assert::AreEqual(0, CountingType::Alive);
			}
			Assert::AreEqual(0, CountingType::Alive);
		}

		TEST_METHOD(AssignOverloads)
		{
			pmon::util::cnr::FixedVector<int, 5> vec;
			vec.Assign(3, 42);
			Assert::AreEqual<size_t>(3u, vec.Size());
			Assert::AreEqual(42, vec[0]);
			Assert::AreEqual(42, vec[2]);

			vec.Assign({ 1, 2 });
			Assert::AreEqual<size_t>(2u, vec.Size());
			Assert::AreEqual(1, vec[0]);
			Assert::AreEqual(2, vec[1]);

			std::array<int, 3> source{ 9, 8, 7 };
			vec.Assign(source.begin(), source.end());
			Assert::AreEqual<size_t>(3u, vec.Size());
			Assert::AreEqual(9, vec[0]);
			Assert::AreEqual(8, vec[1]);
			Assert::AreEqual(7, vec[2]);
		}

		TEST_METHOD(ElementAccessAndIterators)
		{
			pmon::util::cnr::FixedVector<int, 4> vec{ 1, 2, 3 };
			const auto& cvec = vec;

			Assert::AreEqual(1, vec.Front());
			Assert::AreEqual(3, vec.Back());
			Assert::AreEqual(2, vec[1]);
			Assert::AreEqual(2, cvec[1]);
			Assert::AreEqual(1, cvec.At(0));
			Assert::AreEqual(3, cvec.At(2));

			Assert::IsTrue(vec.Data() == &vec[0]);
			Assert::IsTrue(vec.Data() + 2 == &vec[2]);

			int sum = 0;
			for (int value : vec) {
				sum += value;
			}
			Assert::AreEqual(6, sum);

			std::vector<int> reversed;
			for (auto it = vec.RBegin(); it != vec.REnd(); ++it) {
				reversed.push_back(*it);
			}
			Assert::AreEqual<size_t>(3u, reversed.size());
			Assert::AreEqual(3, reversed[0]);
			Assert::AreEqual(2, reversed[1]);
			Assert::AreEqual(1, reversed[2]);
		}

		TEST_METHOD(ExceptionsAndCapacity)
		{
			pmon::util::cnr::FixedVector<int, 3> vec;
			vec.PushBack(1);
			vec.PushBack(2);
			vec.PushBack(3);

			Assert::ExpectException<std::out_of_range>([&]() {
				vec.At(3);
			});

			Assert::ExpectException<std::length_error>([&]() {
				vec.Reserve(4);
			});

			Assert::ExpectException<std::length_error>([&]() {
				vec.Resize(4);
			});

			Assert::ExpectException<std::length_error>([&]() {
				vec.PushBack(4);
			});

			pmon::util::cnr::FixedVector<int, 3> fromInit;
			Assert::ExpectException<std::length_error>([&]() {
				fromInit.Assign({ 1, 2, 3, 4 });
			});

			std::vector<int> sizedRange{ 1, 2, 3, 4 };
			pmon::util::cnr::FixedVector<int, 3> fromSized;
			Assert::ExpectException<std::length_error>([&]() {
				fromSized.Assign(std::from_range, sizedRange);
			});

			CounterRange unsized{ 0, 5 };
			pmon::util::cnr::FixedVector<int, 3> fromUnsized;
			Assert::ExpectException<std::length_error>([&]() {
				fromUnsized.Assign(std::from_range, unsized);
			});
			Assert::AreEqual<size_t>(3u, fromUnsized.Size());
			Assert::AreEqual(0, fromUnsized[0]);
			Assert::AreEqual(1, fromUnsized[1]);
			Assert::AreEqual(2, fromUnsized[2]);
		}

		TEST_METHOD(ReserveAndShrinkNoOp)
		{
			pmon::util::cnr::FixedVector<int, 4> vec{ 1, 2 };
			vec.Reserve(4);
			vec.ShrinkToFit();
			Assert::AreEqual<size_t>(2u, vec.Size());
			Assert::AreEqual(2, vec.Back());
		}
	};
}
