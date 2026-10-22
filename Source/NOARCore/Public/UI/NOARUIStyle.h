#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Fonts/SlateFontInfo.h"
#include "GameplayTagContainer.h"

/**
 * NOAR UI style (Standard v1.1, section 11): dark glass panels, warm white text, bronze accent.
 * Everything is built in code so the prototype has a working, consistent UI before any WBP_ exists.
 * A WBP_ subclass can still override any panel class through ANOARPlayerController::HUDClass.
 */
namespace NOARUI
{
	inline FLinearColor Panel()        { return FLinearColor(0.035f, 0.035f, 0.038f, 0.88f); }
	inline FLinearColor PanelLine()    { return FLinearColor(1.f, 1.f, 1.f, 0.08f); }
	inline FLinearColor Button()       { return FLinearColor(1.f, 1.f, 1.f, 0.06f); }
	inline FLinearColor ButtonHover()  { return FLinearColor(1.f, 1.f, 1.f, 0.14f); }
	inline FLinearColor ButtonPress()  { return FLinearColor(1.f, 1.f, 1.f, 0.22f); }
	inline FLinearColor Accent()       { return FLinearColor(0.72f, 0.54f, 0.32f, 1.f); }   // aged bronze MT01
	inline FLinearColor AccentDim()    { return FLinearColor(0.72f, 0.54f, 0.32f, 0.35f); }
	inline FLinearColor Text()         { return FLinearColor(0.96f, 0.95f, 0.92f, 1.f); }
	inline FLinearColor TextDim()      { return FLinearColor(0.96f, 0.95f, 0.92f, 0.55f); }
	inline FLinearColor TextOnAccent() { return FLinearColor(0.06f, 0.05f, 0.04f, 1.f); }
	inline FLinearColor Danger()       { return FLinearColor(0.75f, 0.30f, 0.25f, 1.f); }

	inline FSlateFontInfo Font(int32 Size, const FName& Face = TEXT("Regular"))
	{
		return FCoreStyle::GetDefaultFontStyle(Face, Size);
	}

	inline FSlateBrush Box(const FLinearColor& Fill, float Radius = 10.f, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.OutlineSettings = FSlateBrushOutlineSettings(FVector4(Radius, Radius, Radius, Radius), FSlateColor(Outline), OutlineWidth);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return B;
	}

	inline FButtonStyle ButtonStyle(const FLinearColor& Normal = Button(), const FLinearColor& Hover = ButtonHover(), const FLinearColor& Press = ButtonPress(), float Radius = 8.f)
	{
		FButtonStyle S;
		S.SetNormal(Box(Normal, Radius));
		S.SetHovered(Box(Hover, Radius));
		S.SetPressed(Box(Press, Radius));
		S.SetDisabled(Box(FLinearColor(1.f, 1.f, 1.f, 0.03f), Radius));
		S.SetNormalPadding(FMargin(14.f, 8.f));
		S.SetPressedPadding(FMargin(14.f, 9.f, 14.f, 7.f));
		return S;
	}

	inline FButtonStyle AccentButtonStyle(float Radius = 8.f)
	{
		return ButtonStyle(Accent(), FLinearColor(0.80f, 0.62f, 0.38f, 1.f), FLinearColor(0.62f, 0.46f, 0.26f, 1.f), Radius);
	}

	inline FButtonStyle SelectedButtonStyle(float Radius = 8.f)
	{
		return ButtonStyle(AccentDim(), AccentDim(), AccentDim(), Radius);
	}

	/** Human label from a gameplay tag leaf: "Configurable.WallPanel.Dining" -> "Wall Panel · Dining" */
	inline FText LabelFromTag(const FGameplayTag& Tag)
	{
		TArray<FString> Parts;
		Tag.ToString().ParseIntoArray(Parts, TEXT("."));
		FString Out;
		for (int32 i = 1; i < Parts.Num(); ++i)
		{
			FString P = Parts[i];
			FString Spaced;
			for (int32 c = 0; c < P.Len(); ++c)
			{
				if (c > 0 && FChar::IsUpper(P[c]) && !FChar::IsUpper(P[c - 1])) Spaced += TEXT(" ");
				Spaced += P[c];
			}
			Out += (i > 1 ? TEXT(" · ") : TEXT("")) + Spaced;
		}
		return FText::FromString(Out);
	}
}
