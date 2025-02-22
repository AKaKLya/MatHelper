// Copyright AKaKLya 2024

#pragma once

template <class T>
struct TAccessPrivate
{
	static inline typename T::Type Value;
};

template <class T,typename T::Type Value>
struct TAccessPrivateStub
{
	struct FAccessPrivateStub
	{
		FAccessPrivateStub()
		{
			TAccessPrivate<T>::Value = Value;
		}
	};

	static inline FAccessPrivateStub AccessPrivateStub;
};