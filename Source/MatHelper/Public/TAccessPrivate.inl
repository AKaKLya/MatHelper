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