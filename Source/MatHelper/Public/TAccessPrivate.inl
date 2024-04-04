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

// Protected Member Access 2
#define PROTECTED_MEMBER_ACCESS_FUNCTION_DEFINE(ClassType,MemberName) \
class ClassType##_##MemberName##_Accessor : public ClassType\
{\
public:\
typedef decltype(MemberName) MemberType;\
MemberType Get() { return MemberName; }\
};\
ClassType##_##MemberName##_Accessor::MemberType Get_##ClassType##_##MemberName(ClassType* origin)\
{\
ClassType##_##MemberName##_Accessor* Accessor = (ClassType##_##MemberName##_Accessor*)origin;\
return Accessor->Get();\
}\



template<typename Tag>
struct result {
	/* export it ... */
	typedef typename Tag::type type;
	static type ptr;
};

template<typename Tag>
typename result<Tag>::type result<Tag>::ptr;

template<typename Tag, typename Tag::type p>
class rob : result<Tag> {
	/* fill it ... */
	struct filler {
		filler() { result<Tag>::ptr = p; }
	};
	static filler filler_obj;
};

template<typename Tag, typename Tag::type p>
typename rob<Tag, p>::filler rob<Tag, p>::filler_obj;


/* Access Private Member eg :
*
struct FPackageFlag
{
	typedef uint32 (UPackage::*Type);
};

template struct TAccessPrivateStub<FPackageFlag,&UPackage::PackageFlagsPrivate>;

void SetFlag()
{
	TArray<FAssetData> ObjectsToExport;
	AssetSelectionUtils::GetSelectedAssets(ObjectsToExport);
	for(auto AssetData : ObjectsToExport)
	{
		if(UPackage* Package = AssetData.GetAsset()->GetOutermost(); !Package->HasAnyPackageFlags(PKG_DisallowExport))
		{ 
			Package->*TAccessPrivate<FPackageFlag>::Value = PKG_DisallowExport; 
		}
	}
}
*/

/* Call Private Function
class A {
private:
void f() {
std::cout << "proof!" << std::endl;
}
};

struct Af { typedef void(A::* type)(); };
template class rob<Af, &A::f>;

int main() {
A a;
(a.*result<Af>::ptr)();
}
*/