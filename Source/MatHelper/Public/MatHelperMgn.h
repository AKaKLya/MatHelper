// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "MatHelperMgn.generated.h"

USTRUCT()
struct FNodeButton
{
	GENERATED_BODY()

	// 在插件面板显示的名字，也是保存节点代码的文件名称.
	// 例如: 填写"ParticleColor"，插件面板的按钮名字会是 "ParticleColor"
	// 在配置文件夹里使用 ParticleColor.txt 保存这个节点的代码.
	UPROPERTY(EditAnywhere,Category = "Material")
	FString ButtonName = "None";

	// 覆盖RootOffset
	UPROPERTY(EditAnywhere,Category = "Material")
	bool RootOffsetOverride = false;

	//节点按钮创建节点时，将会使用这个Root偏移值.
	UPROPERTY(EditAnywhere,Category = "Material",meta = (EditCondition = RootOffsetOverride))
	FVector2D RootOffset = FVector2D(-200,160);
};

UCLASS()
class MATHELPER_API UMatHelperMgn : public UDataAsset
{
	UMatHelperMgn();
	GENERATED_BODY()
	
public:
	FString PluginButtonConfigPath;
	
	//材质资产的显示颜色，引擎默认是绿色(64,192,64)
	UPROPERTY(EditAnywhere,Category = "Material  - 不要删除这个文件.")
	FColor MaterialAssetColor = FColor(255,25,25);

	// 决定了插件助手面板的占比，1是占一半，2是全占. 0.5是一半的一半
	// 更改后需要重新启动 材质编辑器 才会生效
	UPROPERTY(EditAnywhere,Category = "Material  - 不要删除这个文件.")
	float HeightRatio = 1.0;

	// 节点按钮创建节点的位置---相对于Root根节点的位置偏移
	// 但可以勾选 RootOffsetOverride 覆盖这个值.
	UPROPERTY(EditAnywhere,Category = "Material  - 不要删除这个文件.")
	FVector2D RootOffset = FVector2D(-100,800);

	// 节点按钮创建节点的位置---相对于普通节点的位置偏移
	UPROPERTY(EditAnywhere,Category = "Material  - 不要删除这个文件.")
	FVector2D BaseOffset = FVector2D(50,50);

	// 打开节点配置文件夹
	UFUNCTION(CallInEditor,Category = "Material  - 不要删除这个文件.")
	void OpenNodesConfigFolder();

	// 编辑节点文本
	UFUNCTION(CallInEditor,Category = "Material  - 不要删除这个文件.")
	void EditButtonInfo();

	// 刷新节点按钮
	UFUNCTION(CallInEditor,Category = "Material  - 不要删除这个文件.")
	void RefreshHelpersButton();

	// 重新启动UE引擎
	UFUNCTION(CallInEditor,Category = "Material  - 不要删除这个文件.")
	void RestartEditor();

	// 自动分组关键词
	UPROPERTY(EditAnywhere,Category = "Material  - 不要删除这个文件.")
	TArray<FString> AutoGroupKeys;

	// 节点按钮信息
	UPROPERTY(EditAnywhere,Category = "Material  - 不要删除这个文件.")
	TArray<FNodeButton> NodeButtonInfo;
};