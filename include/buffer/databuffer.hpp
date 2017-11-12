#pragma once
#include<cstdint>
#include<stdexcept>
#include<string.h>
#include "endianconversion.hpp"

class DataBuffer
{
public:
	DataBuffer() :bufeer_(nullptr), capacity_(0), r_pos_(0), w_pos_(0), copy_data_(true) {}
	DataBuffer(uint32_t capacity) :bufeer_((uint8_t*)malloc(capacity)), capacity_(capacity), r_pos_(0), w_pos_(0), copy_data_(true)
	{
		if (bufeer_ == nullptr&&capacity!=0)
		{
			throw std::runtime_error("DataBuffer Constructor exception: Memory allocation failure");
		}
	}
	DataBuffer(uint8_t* data, uint32_t size, bool copy_data = true, bool write_pos_to_end = true) :copy_data_(copy_data)
	{
		if (copy_data_)
		{
			capacity_ = size;
			r_pos_ = 0;
			ExtendTo(capacity_);
			Write(data, size);
			w_pos_ = write_pos_to_end ? size : 0;
		}
		else
		{
			bufeer_ = data;
			capacity_ = size;
			r_pos_ = 0;
			w_pos_ = write_pos_to_end ? size : 0;
		}
	}


	DataBuffer(const DataBuffer &rhs)
	{
		bufeer_ = nullptr;
		capacity_ = rhs.capacity_;

		r_pos_ = rhs.r_pos_;
		w_pos_ = rhs.w_pos_;

		copy_data_ = rhs.copy_data_;
		ExtendTo(capacity_);

		memcpy(bufeer_, rhs.bufeer_, rhs.w_pos_);
	}

	DataBuffer& operator=(const DataBuffer &rhs)
	{
		if (&rhs != this)
		{
			Free();
			capacity_ = rhs.capacity_;

			r_pos_ = rhs.r_pos_;
			w_pos_ = rhs.w_pos_;

			copy_data_ = rhs.copy_data_;
			ExtendTo(capacity_);
			memcpy(bufeer_, rhs.bufeer_, rhs.w_pos_);
		}
		return *this;
	}

	DataBuffer(DataBuffer &&rhs)
	{
	
		capacity_ = rhs.capacity_;
		r_pos_ = rhs.r_pos_;
		w_pos_ = rhs.w_pos_;
		copy_data_ = rhs.copy_data_;
		bufeer_ = rhs.bufeer_;

		rhs.capacity_ = rhs.r_pos_ = rhs.w_pos_ = 0;
		rhs.bufeer_ = nullptr;
	}

	DataBuffer& operator=(DataBuffer &&rhs)
	{
		if (&rhs != this)
		{
			Free();
			capacity_ = rhs.capacity_;
			r_pos_ = rhs.r_pos_;
			w_pos_ = rhs.w_pos_;
			copy_data_ = rhs.copy_data_;
			bufeer_ = rhs.bufeer_;

			rhs.capacity_ = rhs.r_pos_ = rhs.w_pos_ = 0;
			rhs.bufeer_ = nullptr;
		}
		return *this;
	}


	virtual ~DataBuffer()
	{
		Free();
	}
public:
	uint32_t Write(void* buf, uint32_t len);
	uint32_t Read(void* buf, uint32_t len);

	template<typename T>
	void WriteIntegerLE(T value)
	{
		static_assert(std::is_integral<T>::value, "must be Integer .");

		endian::HToLe(value);
		Write(&value, sizeof(value));
	}

	template<typename T>
	void ReadIntegerLE(T& value)
	{
		static_assert(std::is_integral<T>::value, "must be Integer .");
		Read(&value, sizeof(value));
		endian::LeToH(value);
	}


	template<typename T>
	void WriteIntegerBE(T value)
	{
		static_assert(std::is_integral<T>::value, "must be Integer .");
		endian::HToBe(value);
		Write(&value, sizeof(value));
	}


	template<typename T>
	void ReadIntegerBE(T& value)
	{
		static_assert(std::is_integral<T>::value, "must be Integer .");
		Read(&value, sizeof(value));
		endian::BeToH(value);
	}

	template<typename ARRAY>
	typename std::enable_if<std::is_array<ARRAY>::value, void>::type
		WriteArray(const ARRAY& a)
	{
		static_assert(std::is_array<ARRAY>::value &&
			(std::is_same<int8_t, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<int8_t, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<uint8_t, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<uint8_t, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<char, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<char, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<unsigned char, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<unsigned char, typename std::remove_extent<ARRAY>::type>::value
			), "must be int8_t or uint8_t or char or unsigned char Array  .");

		Write((typename std::decay<ARRAY>::type)a, std::extent<ARRAY>::value);
	}

	template<typename ARRAY>
	typename std::enable_if<std::is_array<ARRAY>::value, void>::type
		ReadArray(ARRAY& a)
	{
		static_assert(std::is_array<ARRAY>::value &&
			(std::is_same<int8_t, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<int8_t, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<uint8_t, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<uint8_t, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<char, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<char, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<unsigned char, typename std::remove_extent<ARRAY>::type>::value ||
				std::is_same<unsigned char, typename std::remove_extent<ARRAY>::type>::value
				), "must be int8_t or uint8_t or char or unsigned char Array  .");

		Read((typename std::decay<ARRAY>::type)a, std::extent<ARRAY>::value);
	}

	template<typename POD>
	void WritePod(const POD& pod)
	{
		static_assert(std::is_pod<POD>::value/*||std::is_trivial<POD>::value*/, "must be pod ");
		Write(const_cast<POD*>(&pod), sizeof(pod));
	}

	template<typename POD>
	void ReadPod(POD& pod)
	{
		static_assert(std::is_pod<POD>::value/*||std::is_trivial<POD>::value*/, "must be pod ");
		Read(&pod, sizeof(pod));
	}


	void Adjustment();

	uint8_t* GetReadPtr() { return bufeer_ + r_pos_; }
	void SetReadPtr(uint8_t* ptr)
	{
		if (ptr >= bufeer_&&ptr <= bufeer_ + capacity_)
		{
			r_pos_ = uint32_t(ptr - bufeer_);
		}
		else
		{
			throw std::runtime_error("SetReadPtr exception:Invalid address");
		}
	};
	uint32_t GetReadPos() { return r_pos_; }
	void SetReadPos(uint32_t pos)
	{
		if (pos <= capacity_)
		{
			r_pos_ = pos;
		}
		else
		{
			throw std::runtime_error("SetReadPos exception:Invalid pos");
		}
	}
	uint8_t* GetWritePtr() { return bufeer_ + w_pos_; }
	void SetWritePtr(uint8_t* ptr)
	{
		if (ptr >= bufeer_&&ptr <= bufeer_ + capacity_)
		{
			w_pos_ = (uint32_t)(ptr - bufeer_);
		}
		else
		{
			throw std::runtime_error("SetReadPtr exception:Invalid address");
		}
	};
	uint32_t GetWritePos() { return w_pos_; }
	void SetWritePos(uint32_t pos)
	{
		if (pos <= capacity_)
		{
			w_pos_ = pos;
		}
		else
		{
			throw std::runtime_error("SetReadPos exception:Invalid pos");
		}
	}

	uint32_t GetDataSize() { return  w_pos_ - r_pos_; }
	uint32_t GetCapacitySize() { return capacity_; }
	uint32_t GetAvailableSize() { return capacity_ - w_pos_; }
	void SetCapacitySize(uint32_t size)
	{

		capacity_ = size;
		ExtendTo(capacity_);
		if (w_pos_ > size)
			w_pos_ = size;
		if (r_pos_ > size)
			r_pos_ = size;

	}
protected:
	void Extend(uint32_t len);
	void ExtendTo(uint32_t len)
	{
		if (!copy_data_)
			throw std::runtime_error("ExtendTo exception:Automatic allocation of memory is not supported");

		uint8_t* new_buf = (uint8_t*)realloc(bufeer_, len);
		if (new_buf != nullptr)
		{
			bufeer_ = new_buf;
		}
		else
		{
			throw std::runtime_error("ExtendTo exception:Memory allocation failure");
		}
	}
	void Free()
	{
		if (copy_data_&&bufeer_ != nullptr)
		{
			free(bufeer_);
			bufeer_ = nullptr;
			capacity_ = 0;
			w_pos_ = 0;
			r_pos_ = 0;
		}
	}
private:
	uint8_t* bufeer_;//数据指针
	uint32_t capacity_;//申请的空间大小

	uint32_t r_pos_;//读位置
	uint32_t w_pos_;//写位置

	bool copy_data_;
};

inline uint32_t DataBuffer::Write(void* buf, uint32_t len)
{
	if (w_pos_ + len > capacity_)
	{
		Extend(len);
	}

	if (buf != nullptr)
	{
		memcpy(bufeer_ + w_pos_, buf, len);
	}

	w_pos_ += len;

	return len;
}

inline uint32_t DataBuffer::Read(void* buf, uint32_t len)
{
	if (r_pos_ + len > w_pos_)
		throw std::runtime_error("Read exception:Read data length exceeds buffer size");

	if (buf != nullptr)
		memcpy(buf, bufeer_ + r_pos_, len);

	r_pos_ += len;

	return len;
}



inline void DataBuffer::Adjustment()
{
	if (r_pos_ != 0)
	{
		memmove(bufeer_, bufeer_ + r_pos_, w_pos_ - r_pos_);
		w_pos_ -= r_pos_;
		r_pos_ = 0;
	}
}

inline void DataBuffer::Extend(uint32_t len)
{
	capacity_ = w_pos_ + len;
	capacity_ += capacity_ >> 2;
	ExtendTo(capacity_);
}
