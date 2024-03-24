// Copyright AKaKLya 2024

#pragma once

#include "CoreMinimal.h"
#include "MatHelperMgn.generated.h"

USTRUCT()
struct FNodeButton
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere,Category = "Material")
	FString ButtonName = "None";
	
	UPROPERTY(EditAnywhere,Category = "Material")
	bool RootOffsetOverride = false;
	
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
	
	UPROPERTY(EditAnywhere,Category = "Material")
	FColor MaterialAssetColor = FColor(255,25,25);
	
	UPROPERTY(EditAnywhere,Category = "Material")
	FVector2D RootOffset = FVector2D(-100,800);

	UPROPERTY(EditAnywhere,Category = "Material")
	FVector2D BaseOffset = FVector2D(50,50);

	UFUNCTION(CallInEditor,Category = "Material")
	void OpenNodesConfigFolder();
	
	UFUNCTION(CallInEditor,Category = "Material")
	void EditButtonInfo();

	UFUNCTION(CallInEditor,Category = "Material")
	void RefreshHelpersButton();

	UPROPERTY(EditAnywhere,Category = "Material")
	TArray<FString> AutoGroupKeys;
	
	UPROPERTY(EditAnywhere,Category = "Material")
	TArray<FNodeButton> NodeButtonInfo;
};