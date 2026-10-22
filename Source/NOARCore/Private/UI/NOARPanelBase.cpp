#include "UI/NOARPanelBase.h"
#include "UI/NOARUIStyle.h"
#include "Core/NOARPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Spacer.h"

#define LOCTEXT_NAMESPACE "NOARUI"

// ---------------------------------------------------------------- UNOARIdButton

UNOARIdButton::UNOARIdButton()
{
	SetStyle(NOARUI::ButtonStyle());
	SetCursor(EMouseCursor::Hand);
}

TSharedRef<SWidget> UNOARIdButton::RebuildWidget()
{
	if (!OnClicked.IsAlreadyBound(this, &UNOARIdButton::HandleClicked))
	{
		OnClicked.AddDynamic(this, &UNOARIdButton::HandleClicked);
	}
	return Super::RebuildWidget();
}

void UNOARIdButton::HandleClicked()
{
	OnPressedId.Broadcast(Id, Tag);
}

void UNOARIdButton::SetSelected(bool bSelected)
{
	SetStyle(bSelected ? NOARUI::SelectedButtonStyle() : NOARUI::ButtonStyle());
}

// ---------------------------------------------------------------- UNOARPanelBase

TSharedRef<SWidget> UNOARPanelBase::RebuildWidget()
{
	if (!bBuilt && WidgetTree)
	{
		bBuilt = true;
		Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// dim backdrop for modal panels (click-through is intentionally blocked)
		if (bModal)
		{
			UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
			Dim->SetBrush(NOARUI::Box(FLinearColor(0.f, 0.f, 0.f, 0.35f), 0.f));
			UOverlaySlot* DS = Root->AddChildToOverlay(Dim);
			DS->SetHorizontalAlignment(HAlign_Fill);
			DS->SetVerticalAlignment(VAlign_Fill);
		}

		Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
		Card->SetBrush(NOARUI::Box(NOARUI::Panel(), 14.f, NOARUI::PanelLine(), 1.f));
		Card->SetPadding(FMargin(22.f, 18.f, 22.f, 20.f));
		UOverlaySlot* CS = Root->AddChildToOverlay(Card);
		CS->SetHorizontalAlignment(bDockRight ? HAlign_Right : HAlign_Center);
		CS->SetVerticalAlignment(bDockRight ? VAlign_Fill : VAlign_Center);
		CS->SetPadding(bDockRight ? FMargin(0.f, 24.f, 24.f, 24.f) : FMargin(0.f));

		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
		if (CardWidth > 0.f) Size->SetWidthOverride(CardWidth);
		Card->SetContent(Size);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
		Size->SetContent(Column);

		// header: title + close
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
		TitleText = MakeText(Title, 20, NOARUI::Text(), TEXT("Bold"));
		UHorizontalBoxSlot* TS = Header->AddChildToHorizontalBox(TitleText);
		TS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TS->SetVerticalAlignment(VAlign_Center);

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Close"));
		CloseButton->SetStyle(NOARUI::ButtonStyle(FLinearColor::Transparent, NOARUI::ButtonHover(), NOARUI::ButtonPress(), 16.f));
		CloseButton->SetCursor(EMouseCursor::Hand);
		CloseButton->OnClicked.AddDynamic(this, &UNOARPanelBase::HandleCloseClicked);
		UTextBlock* X = MakeText(FText::FromString(TEXT("✕")), 14, NOARUI::TextDim());
		CloseButton->SetContent(X);
		UHorizontalBoxSlot* XS = Header->AddChildToHorizontalBox(CloseButton);
		XS->SetVerticalAlignment(VAlign_Center);
		UVerticalBoxSlot* HS = Column->AddChildToVerticalBox(Header);
		HS->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

		Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Scroll"));
		Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		Scroll->SetAnimateWheelScrolling(true);
		UVerticalBoxSlot* SS = Column->AddChildToVerticalBox(Scroll);
		SS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
		Scroll->AddChild(Content);

		BuildContent();
		SetRenderOpacity(0.f);
		SetVisibility(ESlateVisibility::Collapsed);
	}
	return Super::RebuildWidget();
}

void UNOARPanelBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float Cur = GetRenderOpacity();
	if (!FMath::IsNearlyEqual(Cur, FadeTarget, 0.005f))
	{
		SetRenderOpacity(FMath::FInterpTo(Cur, FadeTarget, InDeltaTime, FadeSpeed));
	}
	else if (FadeTarget <= 0.f && GetVisibility() != ESlateVisibility::Collapsed)
	{
		SetRenderOpacity(0.f);
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNOARPanelBase::Open()
{
	if (bOpen) return;
	if (!bBuilt) TakeWidget();
	bOpen = true;
	Refresh();
	SetVisibility(OpenVisibility);
	FadeTarget = 1.f;
	if (bModal)
	{
		if (ANOARPlayerController* PC = GetNOARController()) PC->SetUIModal(true);
	}
}

void UNOARPanelBase::Close()
{
	if (!bOpen) return;
	bOpen = false;
	FadeTarget = 0.f;
	if (bModal)
	{
		if (ANOARPlayerController* PC = GetNOARController()) PC->SetUIModal(false);
	}
	OnClosed.Broadcast(PanelName);
}

void UNOARPanelBase::HandleCloseClicked()
{
	Close();
}

ANOARPlayerController* UNOARPanelBase::GetNOARController() const
{
	return Cast<ANOARPlayerController>(GetOwningPlayer());
}

// ---------------------------------------------------------------- helpers

UTextBlock* UNOARPanelBase::MakeText(const FText& Text, int32 Size, const FLinearColor& Color, const FName& Face)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(Text);
	T->SetFont(NOARUI::Font(Size, Face));
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetAutoWrapText(true);
	return T;
}

UNOARIdButton* UNOARPanelBase::MakeButton(const FText& Label, FName Id, FGameplayTag Tag, bool bAccent, int32 FontSize)
{
	UNOARIdButton* B = WidgetTree->ConstructWidget<UNOARIdButton>(UNOARIdButton::StaticClass());
	B->Id = Id;
	B->Tag = Tag;
	if (bAccent) B->SetStyle(NOARUI::AccentButtonStyle());
	UTextBlock* T = MakeText(Label, FontSize, bAccent ? NOARUI::TextOnAccent() : NOARUI::Text());
	T->SetAutoWrapText(false);
	B->SetContent(T);
	return B;
}

UBorder* UNOARPanelBase::MakeSection(const FText& Heading)
{
	UVerticalBoxSlot* S;
	UTextBlock* H = MakeText(Heading, 11, NOARUI::TextDim(), TEXT("Bold"));
	S = Content->AddChildToVerticalBox(H);
	S->SetPadding(FMargin(2.f, 14.f, 0.f, 6.f));
	UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Body->SetBrush(NOARUI::Box(FLinearColor(1.f, 1.f, 1.f, 0.025f), 10.f));
	Body->SetPadding(FMargin(10.f));
	S = Content->AddChildToVerticalBox(Body);
	S->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	return Body;
}

UHorizontalBox* UNOARPanelBase::MakeRow()
{
	return WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
}

UImage* UNOARPanelBase::MakeSwatch(const FLinearColor& Color, float Size)
{
	UImage* I = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	FSlateBrush B = NOARUI::Box(Color, 6.f, FLinearColor(1.f, 1.f, 1.f, 0.15f), 1.f);
	B.ImageSize = FVector2D(Size, Size);
	I->SetBrush(B);
	return I;
}

#undef LOCTEXT_NAMESPACE
