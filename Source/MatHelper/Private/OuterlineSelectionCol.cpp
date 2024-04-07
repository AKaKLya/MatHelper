// Copyright AKaKLya 2024


#include "OuterlineSelectionCol.h"

#include "ActorTreeItem.h"
#include "ISceneOutlinerTreeItem.h"
#include "MatHelper.h"
#include "NiagaraActor.h"
#include "ButtonClass/SimpleButtonStyle.h"


SHeaderRow::FColumn::FArguments FOuterlineSelectionLockCol::ConstructHeaderRowColumn()
{
	const ISlateStyle* Style = &FSimpleButtonStyle::Get();
	return SHeaderRow::Column(GetColumnID())
	.FixedWidth(24.f)
	.HAlignHeader(HAlign_Center)
	.VAlignHeader(VAlign_Center)
	.HAlignCell(HAlign_Center)
	.VAlignCell(VAlign_Center)
	.DefaultTooltip(FText::FromString(TEXT("Niagara Lock")))
	[
		SNew(SImage)
		.ColorAndOpacity(FSlateColor::UseForeground())
		.DesiredSizeOverride(FVector2D(20,20))
		.Image(Style->GetBrush("SimpleButton.Niagara"))
	];
	
}

const TSharedRef<SWidget> FOuterlineSelectionLockCol::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem,
	const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	FActorTreeItem* ActorTreeItem = TreeItem->CastTo<FActorTreeItem>();
	if(!ActorTreeItem || !ActorTreeItem->IsValid() || !Cast<ANiagaraActor>(ActorTreeItem->Actor.Get())) return SNullWidget::NullWidget;
	
	const bool bIsActorLocked = ActorTreeItem->Actor.Get()->Tags.Contains("NiagaraAutoPlay");
	
	TSharedRef<SCheckBox> CheckBox = SNew(SCheckBox)
	.Visibility(EVisibility::Visible)
	.HAlign(HAlign_Center)
	.IsChecked(bIsActorLocked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
	.OnCheckStateChanged(this,&FOuterlineSelectionLockCol::OnCheckStateChanged,ActorTreeItem->Actor);
	
	return CheckBox;
}

void FOuterlineSelectionLockCol::OnCheckStateChanged(ECheckBoxState NewState, TWeakObjectPtr<AActor> Actor)
{
	FMatHelperModule& MatHelper = FMatHelperModule::Get();
	
	switch (NewState)
	{
	case ECheckBoxState::Unchecked:
		Actor.Get()->Tags.Remove("NiagaraAutoPlay");
		break;

	case ECheckBoxState::Checked:
		Actor.Get()->Tags.Add("NiagaraAutoPlay");
		break;

	case ECheckBoxState::Undetermined:
		break;

	default:
		break;
	}
}
