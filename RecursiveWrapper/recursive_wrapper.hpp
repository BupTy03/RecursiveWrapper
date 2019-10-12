#pragma once
#ifndef RECURSIVE_WRAPPER_HPP
#define RECURSIVE_WRAPPER_HPP

#include <cstdint>
#include <type_traits>
#include <iostream>
#include <cassert>


namespace impl {
	template<typename T, std::size_t SizeOfStorage, std::size_t AlignOfStorage>
	static constexpr bool use_internal_storage_v =
		std::is_nothrow_move_constructible_v<T> &&
		(sizeof(T) <= SizeOfStorage) &&
		(AlignOfStorage % alignof(T) == 0);
}

template<typename T, std::size_t FixedStorageSize = 256>
class recursive_wrapper {
public:
	recursive_wrapper()
	{
		storage_strategy_ = impl::use_internal_storage_v<T, sizeof(decltype(storage_)), alignof(decltype(storage_))>
			? &internal_storage_strategy : &external_storage_strategy;

		storage_strategy_(storage_, storage_operation::CONSTRUCT, nullptr);
	}

	recursive_wrapper(const recursive_wrapper& other)
	{
		storage_strategy_(storage_, storage_operation::CONSTRUCT_COPY_VALUE, other.get_pointer());
	}
	recursive_wrapper& operator=(const recursive_wrapper& other)
	{
		if (this == &other) {
			return *this;
		}

		storage_strategy_(storage_, storage_operation::ASSIGN_COPY_VALUE, other.get_pointer());
		return *this;
	}

	recursive_wrapper(recursive_wrapper&& other)
	{
		storage_strategy_(storage_, storage_operation::CONSTRUCT_MOVE_VALUE, other.get_pointer());
	}
	recursive_wrapper& operator=(recursive_wrapper&& other)
	{
		if (this == &other) {
			return *this;
		}

		storage_strategy_(storage_, storage_operation::ASSIGN_MOVE_VALUE, other.get_pointer());
		return *this;
	}

	~recursive_wrapper() { storage_strategy_(storage_, storage_operation::DESTROY, nullptr); }

	T& get() { return *get_pointer();}
	T const& get() const { return *get_pointer(); }

	T* get_pointer() 
	{ 
		T* pResult = storage_strategy_(storage_, storage_operation::GET_VALUE_PTR, nullptr);
		assert(pResult);
		return pResult;
	}

	const T* get_pointer() const 
	{
		const T* pResult = storage_strategy_(storage_, storage_operation::GET_VALUE_PTR, nullptr);
		assert(pResult);
		return pResult; 
	}

	operator const T& () const { return get(); }
	operator T& () { return get(); }

private:
	enum class storage_operation {
		CONSTRUCT,
		CONSTRUCT_COPY_VALUE,
		CONSTRUCT_MOVE_VALUE,
		DESTROY,
		GET_VALUE_PTR,
		ASSIGN_COPY_VALUE,
		ASSIGN_MOVE_VALUE,
	};

	union storage_type {
		std::byte internal_storage_[FixedStorageSize];
		T* external_storage_ = nullptr;
	};

	static T* internal_storage_strategy(storage_type& storage, storage_operation op, T* pValue = nullptr)
	{
		std::cout << "internal_storage_strategy" << std::endl;
		switch (op)
		{
		case storage_operation::GET_VALUE_PTR: {
			return reinterpret_cast<T*>(storage.internal_storage_);
		}
		case storage_operation::ASSIGN_COPY_VALUE: {
			assert(pValue != nullptr);
			T* ptr = reinterpret_cast<T*>(storage.internal_storage_);
			ptr->~T();
			::new (ptr) T(*pValue);
			break;
		}
		case storage_operation::ASSIGN_MOVE_VALUE: {
			assert(pValue != nullptr);
			T* ptr = reinterpret_cast<T*>(storage.internal_storage_);
			ptr->~T();
			::new (ptr) T(std::move(*pValue));
			break;
		}
		case storage_operation::CONSTRUCT_COPY_VALUE: {
			assert(pValue != nullptr);
			::new (reinterpret_cast<T*>(storage.internal_storage_)) T(*pValue);
			break;
		}
		case storage_operation::CONSTRUCT_MOVE_VALUE: {
			assert(pValue != nullptr);
			::new (reinterpret_cast<T*>(storage.internal_storage_)) T(std::move(*pValue));
			break;
		}
		case storage_operation::CONSTRUCT: {
			::new (reinterpret_cast<T*>(storage.internal_storage_)) T();
			break;
		}
		case storage_operation::DESTROY: {
			reinterpret_cast<T*>(storage.internal_storage_)->~T();
			break;
		}
		}

		return nullptr;
	}

	static T* external_storage_strategy(storage_type& storage, storage_operation op, T* pValue = nullptr)
	{
		std::cout << "external_storage_strategy" << std::endl;
		switch (op)
		{
		case storage_operation::GET_VALUE_PTR: {
			assert(storage.external_storage_ != nullptr);
			return storage.external_storage_;
		}
		case storage_operation::ASSIGN_COPY_VALUE: {
			assert(storage.external_storage_ != nullptr);
			assert(pValue != nullptr);
			storage.external_storage_->~T();
			::new (storage.external_storage_) T(*pValue);
			break;
		}
		case storage_operation::ASSIGN_MOVE_VALUE: {
			assert(storage.external_storage_ != nullptr);
			assert(pValue != nullptr);
			storage.external_storage_->~T();
			::new (storage.external_storage_) T(std::move(*pValue));
			break;
		}
		case storage_operation::CONSTRUCT_COPY_VALUE: {
			assert(pValue != nullptr);
			storage.external_storage_ = new T(*pValue);
			break;
		}
		case storage_operation::CONSTRUCT_MOVE_VALUE: {
			assert(pValue != nullptr);
			storage.external_storage_ = new T(std::move(*pValue));
			break;
		}
		case storage_operation::CONSTRUCT: {
			storage.external_storage_ = new T();
			break;
		}
		case storage_operation::DESTROY: {
			assert(storage.external_storage_ != nullptr);
			delete storage.external_storage_;
			break;
		}
		}

		return nullptr;
	}

	storage_type storage_;
	T* (*storage_strategy_) (storage_type&, storage_operation, T*) = nullptr;
};

#endif // !RECURSIVE_WRAPPER_HPP
