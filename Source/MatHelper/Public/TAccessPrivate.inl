// Copyright AKaKLya 2024

#pragma once

//----------[Call Private Variable]-------------//
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

/*
class MyClass 
{
public:
	MyClass()=default;
private:
	int ValueA = 0;
};

struct AccessValueA
{
	typedef int (MyClass::*Type);
};

template struct TAccessPrivateStub<AccessValueA,&MyClass::ValueA>;

void AccessValueATemplate() 
{
	MyClass* obj = new MyClass;

	// 获取私有成员变量的指针
	int PrivateVar = obj->*TAccessPrivate<AccessValueA>::Value;

	UE_LOG(LogTemp,Warning,TEXT("ValueA: %d"),PrivateVar);
}
*/

//----------[Call Private Function]-------------//

// 模板类，用于存储私有成员函数的指针
template <typename T, typename FuncPtr>
struct TAccessPrivateFunction 
{
	static inline FuncPtr Value; // 存储私有成员函数的指针
};

// 模板类，用于在静态初始化时设置私有成员函数的指针
template <typename T, typename FuncPtr, FuncPtr Value>
struct TAccessPrivateFunctionStub 
{
	struct FAccessPrivateStub 
	{
		FAccessPrivateStub() 
		{
			TAccessPrivateFunction<T, FuncPtr>::Value = Value; // 在静态初始化时设置指针
		}
	};

	static inline FAccessPrivateStub AccessPrivateStub; // 静态成员，触发构造函数
};

/* 目标类，包含私有成员函数
class MyClass 
{
private:
	void VoidFunc() {
		std::cout << "Private function called!" << std::endl;
	}

	int IntVarFunc(int a)
	{
		cout << "func: " << a << endl;
		return 55;
	}
};

//-----------Call void VoidFunc()-----------//

// 定义私有成员函数的类型
using CallVoidFunc = void (MyClass::*)();

// 特化模板，将私有成员函数的指针存储到 TAccessPrivateFunction 中
template struct TAccessPrivateFunctionStub<MyClass, CallVoidFunc, &MyClass::VoidFunc>;

void VoidFuncTemplate() 
{
	MyClass obj;

	// 获取私有成员函数的指针
	auto pFunc = TAccessPrivateFunction<MyClass, CallVoidFunc>::Value;

	// 调用私有成员函数
	(obj.*pFunc)();
}

//---------Call int IntVarFunc(int a)-----------//

// 定义私有成员函数的类型
using CallIntVarFunc = int (MyClass::*)(int);

// 特化模板，将私有成员函数的指针存储到 TAccessPrivateFunction 中
template struct TAccessPrivateFunctionStub<MyClass, CallIntVarFunc, &MyClass::IntVarFunc>;

void IntVarFuncTemplate() 
{
	MyClass obj;

	// 获取私有成员函数的指针
	auto pFunc = TAccessPrivateFunction<MyClass, CallIntVarFunc>::Value;

	// 调用私有成员函数，并传递参数
	int result = (obj.*pFunc)(10); // 调用 PrivateFunction(10)

	std::cout << "Result: " << result << std::endl;
}

//---------Call int IntVarFunc(int a)-----------//


int main() 
{
	VoidFuncTemplate();
	IntVarFuncTemplate();
	return 0;
}
*/