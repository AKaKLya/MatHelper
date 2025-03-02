// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "MatHelperSettings.generated.h"

#define DECLARE_NODE_MASK_PIN(Channel, R, G, B, A) FNodeMaskPin(FString(#Channel), FIntVector4(R, G, B, A))

UENUM()
enum class ESceneViewMethod : uint8
{
	// 创建SceneView时，自动设置为世界场景的视角.
	Auto,
	// 创建SceneView时，以选中的Actor为视角位置.
	SelectActor,
};

USTRUCT()
struct FNodeMaskPin
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere,Category = "Material")
	FString ButtonName = "";

	// X:R , Y:G , Z:B , W:A
	UPROPERTY(EditAnywhere,Category = "Material")
	FIntVector4 MaskValue = FIntVector4(0,0,0,0);

	FNodeMaskPin(){};
	FNodeMaskPin(const FString& InName, const FIntVector4 InValue)
		:ButtonName(InName),MaskValue(InValue){}
};

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
/**
 * 
 */
UCLASS(config=EditorPerProjectUserSettings)
class MATHELPER_API UMatHelperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()
	
public:
	UMatHelperSettings();
	
	FString PluginButtonConfigPath;
	
	virtual FName GetCategoryName() const override;
	
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> CommonEditorMaps;
	

	// ICON的SVG文件名称，例如:A.svg,只需要填写 A 即可.
	UPROPERTY(config,EditAnywhere,Category = "Material")
	FString IConName;
	
	// 创建SceneView时，选择视角的方式.
	UPROPERTY(config,EditAnywhere,Category = "General")
	ESceneViewMethod SceneViewMethod = ESceneViewMethod::Auto;
	
	//材质资产的显示颜色，引擎默认是绿色(64,192,64)
	UPROPERTY(config,EditAnywhere,Category = "Material")
	FColor MaterialAssetColor = FColor(255,25,25);

	//材质实例资产的显示颜色，引擎默认是绿色(0,128,0)
	UPROPERTY(config,EditAnywhere,Category = "Material")
	FColor MaterialInstanceAssetColor = FColor(0,128,0);

	// 决定了插件助手面板的占比，1是占一半，2是全占. 0.5是一半的一半
	// 更改后需要重新启动 材质编辑器 才会生效
	UPROPERTY(config,EditAnywhere,Category = "Material")
	float HeightRatio = 1.0;

	// 节点按钮创建节点的位置---相对于Root根节点的位置偏移
	// 但可以勾选 RootOffsetOverride 覆盖这个值.
	UPROPERTY(config,EditAnywhere,Category = "Material")
	FVector2D RootOffset = FVector2D(-100,800);

	// 节点按钮创建节点的位置---相对于普通节点的位置偏移
	UPROPERTY(config,EditAnywhere,Category = "Material")
	FVector2D BaseOffset = FVector2D(50,50);

	UPROPERTY(config,EditAnywhere,Category = "Material")
	TArray<FNodeMaskPin> MaskPinInfo;
	
	// 自动分组关键词
	UPROPERTY(config,EditAnywhere,Category = "Material")
	TArray<FString> AutoGroupKeys;

	// 节点按钮信息
	UPROPERTY(config,EditAnywhere,Category = "Material")
	TArray<FNodeButton> NodeButtonInfo ;
	
	// 将Niagara拖入定序器时，自动打开轨道并设置为 DesiredAge 模式.
	// 设置更改后需要重启引擎.
	UPROPERTY(config,EditAnywhere,Category = "Niagara")
	bool OverrideNiagaraSequenceMode = true;

	UPROPERTY(config,EditAnywhere,Category = "Niagara")
	bool CreateNiagaraAutoPlaySelection = true;

	//[PasteNodesHere] 在dll中的偏移值.
	UPROPERTY(config,EditAnywhere,AdvancedDisplay,Category = "AdvanceHook")
	uint64 PasteNodeHereOffset = 723040;

	//[CreateGroupsWidget] 在dll中的偏移值.
	UPROPERTY(config,EditAnywhere,AdvancedDisplay,Category = "AdvanceHook")
	uint64 MIEditorCreateWidgetOffset = 1634912;

	//[NiagaraExtendToolbar] 在dll中的偏移值.
	UPROPERTY(config,EditAnywhere,AdvancedDisplay,Category = "AdvanceHook")
	uint64 NiagaraExtendToolbarOffset = 9653744;

	//[NiagaraRegisterTabFactories] 在dll中的偏移值.
	UPROPERTY(config,EditAnywhere,AdvancedDisplay,Category = "AdvanceHook")
	uint64 NiagaraRegisterTabFactoriesOffset = 9983616;
};


class FMatHelperSettingsCustomization : public IDetailCustomization
{
public:
	FMatHelperSettingsCustomization();
	virtual ~FMatHelperSettingsCustomization() override;

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	// End of IDetailCustomization interface
};