#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace pmon::util::cnr
{
	template<typename T, size_t MaxCapacity>
	class FixedVector
	{
	public:
		using value_type = T;
		using size_type = size_t;
		using difference_type = std::ptrdiff_t;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;
		using iterator = T*;
		using const_iterator = const T*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		FixedVector() noexcept = default;

		explicit FixedVector(size_type count)
		{
			Resize(count);
		}

		FixedVector(size_type count, const T& value)
		{
			Resize(count, value);
		}

		FixedVector(std::initializer_list<T> init)
		{
			Assign(init);
		}

		template<typename It, std::enable_if_t<!std::is_integral_v<It>, int> = 0>
		FixedVector(It first, It last)
		{
			Assign(first, last);
		}

		template<std::ranges::input_range R>
		FixedVector(std::from_range_t, R&& range)
		{
			AssignRange_(std::forward<R>(range));
		}

		FixedVector(const FixedVector& other)
		{
			CopyFrom_(other);
		}

		FixedVector(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			MoveFrom_(std::move(other));
		}

		FixedVector& operator=(const FixedVector& other)
		{
			if (this != &other) {
				Clear();
				CopyFrom_(other);
			}
			return *this;
		}

		FixedVector& operator=(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			if (this != &other) {
				Clear();
				MoveFrom_(std::move(other));
			}
			return *this;
		}

		FixedVector& operator=(std::initializer_list<T> init)
		{
			Assign(init);
			return *this;
		}

		~FixedVector()
		{
			Clear();
		}

		iterator begin() noexcept { return Data(); }
		const_iterator begin() const noexcept { return Data(); }
		const_iterator CBegin() const noexcept { return Data(); }
		iterator end() noexcept { return Data() + size_; }
		const_iterator end() const noexcept { return Data() + size_; }
		const_iterator CEnd() const noexcept { return Data() + size_; }

		reverse_iterator RBegin() noexcept { return reverse_iterator(end()); }
		const_reverse_iterator RBegin() const noexcept { return const_reverse_iterator(end()); }
		const_reverse_iterator CRBegin() const noexcept { return const_reverse_iterator(CEnd()); }
		reverse_iterator REnd() noexcept { return reverse_iterator(begin()); }
		const_reverse_iterator REnd() const noexcept { return const_reverse_iterator(begin()); }
		const_reverse_iterator CREnd() const noexcept { return const_reverse_iterator(CBegin()); }

		size_type Size() const noexcept { return size_; }
		constexpr size_type Capacity() const noexcept { return MaxCapacity; }
		constexpr size_type MaxSize() const noexcept { return MaxCapacity; }
		bool Empty() const noexcept { return size_ == 0; }

		void Reserve(size_type count)
		{
			CheckCapacity_(count);
		}

		void ShrinkToFit() noexcept {}

		reference operator[](size_type index) noexcept { return *Ptr_(index); }
		const_reference operator[](size_type index) const noexcept { return *Ptr_(index); }

		reference At(size_type index)
		{
			if (index >= size_) {
				throw std::out_of_range("FixedVector::At");
			}
			return (*this)[index];
		}

		const_reference At(size_type index) const
		{
			if (index >= size_) {
				throw std::out_of_range("FixedVector::At");
			}
			return (*this)[index];
		}

		reference Front() noexcept
		{
			assert(size_ > 0);
			return (*this)[0];
		}

		const_reference Front() const noexcept
		{
			assert(size_ > 0);
			return (*this)[0];
		}

		reference Back() noexcept
		{
			assert(size_ > 0);
			return (*this)[size_ - 1];
		}

		const_reference Back() const noexcept
		{
			assert(size_ > 0);
			return (*this)[size_ - 1];
		}

		pointer Data() noexcept { return DataPtr_(); }
		const_pointer Data() const noexcept { return DataPtr_(); }

		void Clear() noexcept
		{
			DestroyRange_(0, size_);
			size_ = 0;
		}

		void Resize(size_type count)
		{
			if (count < size_) {
				DestroyRange_(count, size_);
				size_ = count;
			}
			else if (count > size_) {
				CheckCapacity_(count);
				for (size_type i = size_; i < count; ++i) {
					std::construct_at(Ptr_(i));
					++size_;
				}
			}
		}

		void Resize(size_type count, const T& value)
		{
			if (count < size_) {
				DestroyRange_(count, size_);
				size_ = count;
			}
			else if (count > size_) {
				CheckCapacity_(count);
				for (size_type i = size_; i < count; ++i) {
					std::construct_at(Ptr_(i), value);
					++size_;
				}
			}
		}

		void PushBack(const T& value)
		{
			EmplaceBack(value);
		}

		void PushBack(T&& value)
		{
			EmplaceBack(std::move(value));
		}

		template<typename... Args>
		reference EmplaceBack(Args&&... args)
		{
			CheckCapacity_(size_ + 1);
			pointer ptr = Ptr_(size_);
			std::construct_at(ptr, std::forward<Args>(args)...);
			++size_;
			return *ptr;
		}

		void PopBack() noexcept
		{
			assert(size_ > 0);
			if (size_ == 0) {
				return;
			}
			DestroyRange_(size_ - 1, size_);
			--size_;
		}

		void Assign(size_type count, const T& value)
		{
			Clear();
			Resize(count, value);
		}

		void Assign(std::initializer_list<T> init)
		{
			Clear();
			CheckCapacity_(init.size());
			for (const auto& value : init) {
				EmplaceBack(value);
			}
		}

		template<typename It, std::enable_if_t<!std::is_integral_v<It>, int> = 0>
		void Assign(It first, It last)
		{
			Clear();
			for (; first != last; ++first) {
				EmplaceBack(*first);
			}
		}

		template<std::ranges::input_range R>
		void Assign(std::from_range_t, R&& range)
		{
			AssignRange_(std::forward<R>(range));
		}

	private:
		using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;

		template<std::ranges::input_range R>
		void AssignRange_(R&& range)
		{
			Clear();
			if constexpr (std::ranges::sized_range<R>) {
				CheckCapacity_(static_cast<size_type>(std::ranges::size(range)));
			}
			for (auto&& value : range) {
				EmplaceBack(std::forward<decltype(value)>(value));
			}
		}

		pointer DataPtr_() noexcept
		{
			if (size_ == 0) {
				return reinterpret_cast<pointer>(storage_.data());
			}
			return Ptr_(0);
		}

		const_pointer DataPtr_() const noexcept
		{
			if (size_ == 0) {
				return reinterpret_cast<const_pointer>(storage_.data());
			}
			return Ptr_(0);
		}

		pointer Ptr_(size_type index) noexcept
		{
			return std::launder(reinterpret_cast<pointer>(&storage_[index]));
		}

		const_pointer Ptr_(size_type index) const noexcept
		{
			return std::launder(reinterpret_cast<const_pointer>(&storage_[index]));
		}

		void DestroyRange_(size_type first, size_type last) noexcept
		{
			for (size_type i = first; i < last; ++i) {
				std::destroy_at(Ptr_(i));
			}
		}

		void CheckCapacity_(size_type count) const
		{
			if (count > MaxCapacity) {
				throw std::length_error("FixedVector capacity exceeded");
			}
		}

		void CopyFrom_(const FixedVector& other)
		{
			for (size_type i = 0; i < other.size_; ++i) {
				std::construct_at(Ptr_(i), other[i]);
				++size_;
			}
		}

		void MoveFrom_(FixedVector&& other)
		{
			for (size_type i = 0; i < other.size_; ++i) {
				std::construct_at(Ptr_(i), std::move(other[i]));
				++size_;
			}
			other.Clear();
		}

		std::array<Storage, MaxCapacity> storage_{};
		size_type size_ = 0;
	};
}
