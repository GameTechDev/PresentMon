#pragma once
#include <memory>

namespace pmon::util
{
	inline constexpr size_t GetPadding(size_t byteIndex, size_t alignment)
	{
		const auto partialBytes = byteIndex % alignment;
		const auto padding = (alignment - partialBytes) % alignment;
		return padding;
	}

	template<typename T>
	constexpr size_t GetPadding(size_t byteIndex)
	{
		return GetPadding(byteIndex, alignof(T));
	}

	inline constexpr size_t PadToAlignment(size_t bytes, size_t alignment)
	{
		return bytes + GetPadding(bytes, alignment);
	}

	template<std::copy_constructible T>
	class CloningUptr : public std::unique_ptr<T>
	{
	public:
		CloningUptr() = default;
		explicit CloningUptr(T* p) noexcept : std::unique_ptr<T>{ p } {}
		CloningUptr(CloningUptr&& other) noexcept : std::unique_ptr<T>{ std::move(other) } {}
		// TODO: consider slicing implications / metaprogramming detection of clone() member etc.
		CloningUptr(const CloningUptr& other)
			:
			std::unique_ptr<T>{ bool(other) ? std::make_unique<T>(*other) : std::unique_ptr<T>{} }
		{}
		CloningUptr(std::unique_ptr<T>&& up) : std::unique_ptr<T>{ std::move(up) } {}
		CloningUptr& operator=(const CloningUptr& rhs)
		{
			std::unique_ptr<T>::reset(CloningUptr{ rhs }.std::unique_ptr<T>::release());
			return *this;
		}
		CloningUptr& operator=(CloningUptr&& rhs) noexcept
		{
			std::unique_ptr<T>::reset(rhs.release());
			return *this;
		}
		~CloningUptr() = default;
	};

	// get the size in bytes of a container's contents
	template<class C>
	size_t SizeInBytes(const C& container)
	{
		if (const auto size = std::size(container)) {
			return size * sizeof(*std::begin(container));
		}
		return 0;
	}

	// out pointer for using smart pointers with c-api type functions
	template<typename SP>
	class OutPtrProxy {
	public:
		using Pointer = typename SP::pointer;

		explicit OutPtrProxy(SP& smartPtr)
			: smartPtr_(smartPtr), rawPtr_(nullptr)
		{}

		~OutPtrProxy() {
			smartPtr_.reset(rawPtr_);
		}

		// implicit conversion so it can be passed wherever a Pointer* is required
		operator Pointer* () {
			return &rawPtr_;
		}

		// non-copyable
		OutPtrProxy(OutPtrProxy const&) = delete;
		OutPtrProxy& operator=(OutPtrProxy const&) = delete;

		// moveable: steals the rawPtr_, leaves other.rawPtr_ null
		OutPtrProxy(OutPtrProxy&& other) noexcept
			: smartPtr_(other.smartPtr_), rawPtr_(other.rawPtr_)
		{
			other.rawPtr_ = nullptr;
		}
		OutPtrProxy& operator=(OutPtrProxy&& other) noexcept {
			if (this != &other) {
				// first commit any existing rawPtr_
				smartPtr_.reset(rawPtr_);
				// then steal
				rawPtr_ = other.rawPtr_;
				other.rawPtr_ = nullptr;
			}
			return *this;
		}

	private:
		SP& smartPtr_;
		Pointer  rawPtr_;
	};
	template<typename SP>
	OutPtrProxy<SP> OutPtr(SP& smartPtr) {
		return OutPtrProxy<SP>(smartPtr);
	}

	void LocalFree(void* p) noexcept;

	// Generic LocalFree deleter
	template<class T = void>
	struct LocalFreeDeleter
	{
		using pointer = T*;
		void operator()(pointer p) const noexcept
		{
			if (p) {
				util::LocalFree(p);
			}
		}
	};

	// unique_ptr that uses LocalFree.
	template<class T = void>
	using UniqueLocalPtr = std::unique_ptr<T, LocalFreeDeleter<T>>;
}
