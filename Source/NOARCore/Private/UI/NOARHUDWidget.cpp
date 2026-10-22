#include "UI/NOARHUDWidget.h"
#include "UI/NOARPanels.h"
#include "UI/NOARUIStyle.h"
#include "Core/NOARPlayerController.h"
#include "Core/NOARApartmentSubsystem.h"
#include "Character/NOARCharacter.h"
#include "Interaction/NOARInteractionComponent.h"
#include "Data/NOARDataAssets.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "NOARUI"

UNOARHUDWidget::UNOARHUDWidget(const FObjectInitializer& OI) : Super(OI)
{
	FinishesPanelClass = UNOARFinishesPanel::StaticClass();
	RoomsPanelClass = UNOARRoomsPanel::StaticClass();
	LightingPanelClass = UNOARLightingPanel::StaticClass();
	MenuPanelClass = UNOARMenuPanel::StaticClass();
	ProductPanelClass = UNOARProductPanel::StaticClass();
	PhotoOverlayClass = UNOARPhotoOverlay::StaticClass();
}

ANOARPlayerController* UNOARHUDWidget::GetNOARController() const
{
	return Cast<ANOARPlayerController>(GetOwningPlayer());
}

// ---------------------------------------------------------------- build

TSharedRef<SWidget> UNOARHUDWidget::RebuildWidget()
{
	if (!bBuilt && WidgetTree)
	{
		bBuilt = true;
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
		WidgetTree->RootWidget = Canvas;

		auto MakeText = [this](const FText& T, int32 Size, const FLinearColor& C, const FName& Face = TEXT("Regular"))
		{
			UTextBlock* TB = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			TB->SetText(T); TB->SetFont(NOARUI::Font(Size, Face)); TB->SetColorAndOpacity(FSlateColor(C));
			return TB;
		};
		auto Pill = [this](float Radius = 18.f)
		{
			UBorder* B = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			B->SetBrush(NOARUI::Box(NOARUI::Panel(), Radius, NOARUI::PanelLine(), 1.f));
			B->SetPadding(FMargin(14.f, 7.f));
			return B;
		};

		// ---- toolbar, top-right
		Toolbar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Toolbar"));
		for (const auto& P : { TPair<FName, FText>(TEXT("Finishes"), LOCTEXT("TbFinishes", "Finishes  F")), TPair<FName, FText>(TEXT("Rooms"), LOCTEXT("TbRooms", "Rooms  R")),
							   TPair<FName, FText>(TEXT("Lighting"), LOCTEXT("TbLight", "Light  L")), TPair<FName, FText>(TEXT("Photo"), LOCTEXT("TbPhoto", "Photo  P")),
							   TPair<FName, FText>(TEXT("Menu"), LOCTEXT("TbMenu", "Menu  Esc")) })
		{
			UNOARIdButton* B = WidgetTree->ConstructWidget<UNOARIdButton>(UNOARIdButton::StaticClass());
			B->Id = P.Key;
			B->SetStyle(NOARUI::ButtonStyle(NOARUI::Panel(), FLinearColor(0.12f, 0.12f, 0.12f, 0.9f), FLinearColor(0.2f, 0.2f, 0.2f, 0.9f), 16.f));
			UTextBlock* T = MakeText(P.Value, 12, NOARUI::Text());
			B->SetContent(T);
			B->OnPressedId.AddDynamic(this, &UNOARHUDWidget::HandleToolbar);
			UHorizontalBoxSlot* S = Toolbar->AddChildToHorizontalBox(B);
			S->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		}
		UCanvasPanelSlot* TS = Canvas->AddChildToCanvas(Toolbar);
		TS->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
		TS->SetAlignment(FVector2D(1.f, 0.f));
		TS->SetPosition(FVector2D(-24.f, 20.f));
		TS->SetAutoSize(true);

		// ---- interaction prompt (follows the focused part)
		Prompt = Pill(16.f);
		UHorizontalBox* PR = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		PromptKey = MakeText(FText::FromString(TEXT("E")), 11, NOARUI::TextOnAccent(), TEXT("Bold"));
		UBorder* KeyBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		KeyBox->SetBrush(NOARUI::Box(NOARUI::Accent(), 5.f));
		KeyBox->SetPadding(FMargin(6.f, 1.f));
		KeyBox->SetContent(PromptKey);
		UHorizontalBoxSlot* KS = PR->AddChildToHorizontalBox(KeyBox);
		KS->SetVerticalAlignment(VAlign_Center);
		KS->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		PromptText = MakeText(FText::GetEmpty(), 13, NOARUI::Text());
		UHorizontalBoxSlot* PS = PR->AddChildToHorizontalBox(PromptText);
		PS->SetVerticalAlignment(VAlign_Center);
		Prompt->SetContent(PR);
		Prompt->SetRenderOpacity(0.f);
		Prompt->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* PrS = Canvas->AddChildToCanvas(Prompt);
		PrS->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		PrS->SetAlignment(FVector2D(0.5f, 1.f));
		PrS->SetAutoSize(true);

		// ---- hints, bottom-left
		Hints = Pill(12.f);
		HintsText = MakeText(FText::GetEmpty(), 11, NOARUI::TextDim());
		Hints->SetContent(HintsText);
		Hints->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* HS = Canvas->AddChildToCanvas(Hints);
		HS->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		HS->SetAlignment(FVector2D(0.f, 1.f));
		HS->SetPosition(FVector2D(24.f, -20.f));
		HS->SetAutoSize(true);

		// ---- room toast, top-left
		Toast = Pill(12.f);
		ToastText = MakeText(FText::GetEmpty(), 14, NOARUI::Text(), TEXT("Bold"));
		Toast->SetContent(ToastText);
		Toast->SetRenderOpacity(0.f);
		Toast->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* ToS = Canvas->AddChildToCanvas(Toast);
		ToS->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		ToS->SetAlignment(FVector2D(0.f, 0.f));
		ToS->SetPosition(FVector2D(24.f, 20.f));
		ToS->SetAutoSize(true);

		// ---- panel host, full screen, above everything
		PanelHost = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PanelHost"));
		UCanvasPanelSlot* PhS = Canvas->AddChildToCanvas(PanelHost);
		PhS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		PhS->SetOffsets(FMargin(0.f));
		PhS->SetZOrder(10);
	}
	return Super::RebuildWidget();
}

UNOARPanelBase* UNOARHUDWidget::CreatePanel(TSubclassOf<UNOARPanelBase> Class, FName Name)
{
	if (!Class) return nullptr;
	UNOARPanelBase* P = CreateWidget<UNOARPanelBase>(GetOwningPlayer(), Class);
	if (!P) return nullptr;
	P->PanelName = Name;
	P->OnClosed.AddDynamic(this, &UNOARHUDWidget::HandlePanelClosed);
	UOverlaySlot* S = PanelHost->AddChildToOverlay(P);
	S->SetHorizontalAlignment(HAlign_Fill);
	S->SetVerticalAlignment(VAlign_Fill);
	Panels.Add(Name, P);
	return P;
}

UNOARPanelBase* UNOARHUDWidget::FindPanel(FName Name) const
{
	const TObjectPtr<UNOARPanelBase>* P = Panels.Find(Name);
	return P ? P->Get() : nullptr;
}

void UNOARHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Panels.Num() == 0)
	{
		CreatePanel(FinishesPanelClass, TEXT("Finishes"));
		CreatePanel(RoomsPanelClass, TEXT("Rooms"));
		CreatePanel(LightingPanelClass, TEXT("Lighting"));
		CreatePanel(MenuPanelClass, TEXT("Menu"));
		CreatePanel(ProductPanelClass, TEXT("Product"));
		CreatePanel(PhotoOverlayClass, TEXT("Photo"));
	}
	if (ANOARPlayerController* PC = GetNOARController())
	{
		PC->OnUIRequest.AddUniqueDynamic(this, &UNOARHUDWidget::HandleUIRequest);
	}
	BindToWorld();
	UpdateHints();
}

void UNOARHUDWidget::NativeDestruct()
{
	if (ANOARPlayerController* PC = GetNOARController())
	{
		PC->OnUIRequest.RemoveDynamic(this, &UNOARHUDWidget::HandleUIRequest);
	}
	Super::NativeDestruct();
}

void UNOARHUDWidget::BindToWorld()
{
	ANOARPlayerController* PC = GetNOARController();
	ANOARCharacter* C = PC ? PC->GetNOARCharacter() : nullptr;
	if (C && C->Interaction && !bBoundToWorld)
	{
		C->Interaction->OnFocusChanged.AddUniqueDynamic(this, &UNOARHUDWidget::HandleFocusChanged);
		C->OnCameraModeChanged.AddUniqueDynamic(this, &UNOARHUDWidget::HandleCameraMode);
		CameraMode = C->GetCameraMode();
		if (UNOARApartmentSubsystem* Apt = GetWorld()->GetSubsystem<UNOARApartmentSubsystem>())
		{
			Apt->OnRoomEntered.AddUniqueDynamic(this, &UNOARHUDWidget::HandleRoomEntered);
		}
		bBoundToWorld = true;
		UpdateHints();
	}
}

// ---------------------------------------------------------------- tick

void UNOARHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bBoundToWorld) BindToWorld();   // pawn may be possessed after the HUD exists

	// prompt follows the focused part
	if (Prompt)
	{
		const float Cur = Prompt->GetRenderOpacity();
		Prompt->SetRenderOpacity(FMath::FInterpTo(Cur, PromptTarget, InDeltaTime, 10.f));
		if (bHasFocus && !bPhoto)
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				FVector2D Screen;
				if (PC->ProjectWorldLocationToScreen(FocusWorldPoint + FVector(0.f, 0.f, 12.f), Screen, true))
				{
					const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
					if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Prompt->Slot))
					{
						S->SetPosition(Screen / Scale);
					}
				}
			}
		}
	}
	if (Toast && ToastTimer > 0.f)
	{
		ToastTimer -= InDeltaTime;
		Toast->SetRenderOpacity(FMath::Clamp(ToastTimer * 2.f, 0.f, 1.f));
	}
}

// ---------------------------------------------------------------- panels

void UNOARHUDWidget::TogglePanel(FName Panel)
{
	if (Panel == TEXT("Photo"))
	{
		bPhoto = true;
		CloseAllPanels();
		SetChromeVisible(false);
		if (UNOARPanelBase* P = FindPanel(TEXT("Photo"))) P->Open();
		return;
	}
	if (Panel == TEXT("PhotoClosed"))
	{
		bPhoto = false;
		if (UNOARPanelBase* P = FindPanel(TEXT("Photo"))) P->Close();
		SetChromeVisible(true);
		return;
	}
	// Esc (IA_Menu) while any panel is open closes it instead of stacking the menu on top
	if (Panel == TEXT("Menu") && IsAnyPanelOpen())
	{
		CloseAllPanels();
		return;
	}
	UNOARPanelBase* P = FindPanel(Panel);
	if (!P) return;
	if (P->IsOpen())
	{
		P->Close();
		return;
	}
	if (Panel == TEXT("Product"))
	{
		if (UNOARProductPanel* PP = Cast<UNOARProductPanel>(P))
		{
			ANOARPlayerController* PC = GetNOARController();
			FNOARProductInfo Info;
			if (PC && PC->GetNOARCharacter() && PC->GetNOARCharacter()->Interaction->GetFocusedProductInfo(Info)) PP->SetProduct(Info);
		}
	}
	CloseAllPanels();
	P->Open();
	OpenPanel = P;
}

void UNOARHUDWidget::CloseAllPanels()
{
	for (auto& Pair : Panels)
	{
		if (Pair.Value && Pair.Key != TEXT("Photo") && Pair.Value->IsOpen()) Pair.Value->Close();
	}
	OpenPanel = nullptr;
}

bool UNOARHUDWidget::IsAnyPanelOpen() const
{
	for (const auto& Pair : Panels)
	{
		if (Pair.Value && Pair.Key != TEXT("Photo") && Pair.Value->IsOpen()) return true;
	}
	return false;
}

void UNOARHUDWidget::HandleUIRequest(FName Panel)
{
	TogglePanel(Panel);
}

void UNOARHUDWidget::HandleToolbar(FName Id, FGameplayTag)
{
	if (Id == TEXT("Photo"))
	{
		if (ANOARPlayerController* PC = GetNOARController()) PC->SetPhotoMode(true);
		return;
	}
	TogglePanel(Id);
}

void UNOARHUDWidget::HandlePanelClosed(FName Panel)
{
	if (OpenPanel && OpenPanel->PanelName == Panel) OpenPanel = nullptr;
}

void UNOARHUDWidget::SetChromeVisible(bool bVisible)
{
	const ESlateVisibility V = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	const ESlateVisibility H = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (Toolbar) Toolbar->SetVisibility(V);
	if (Hints) Hints->SetVisibility(H);
	if (Prompt) Prompt->SetVisibility(H);
	if (Toast) Toast->SetVisibility(H);
}

// ---------------------------------------------------------------- world events

void UNOARHUDWidget::HandleFocusChanged(AActor* FocusedActor, const FText& PromptIn, FVector WorldPoint, ENOARInteractDenyReason Reason)
{
	bHasFocus = FocusedActor != nullptr && !PromptIn.IsEmpty();
	FocusWorldPoint = WorldPoint;
	PromptTarget = bHasFocus ? 1.f : 0.f;
	if (!PromptText || !bHasFocus) return;
	PromptText->SetText(PromptIn);
	const bool bCan = Reason == ENOARInteractDenyReason::OK;
	PromptText->SetColorAndOpacity(FSlateColor(bCan ? NOARUI::Text() : NOARUI::TextDim()));
	if (UBorder* KeyBox = PromptKey ? Cast<UBorder>(PromptKey->GetParent()) : nullptr)
	{
		KeyBox->SetBrush(NOARUI::Box(bCan ? NOARUI::Accent() : FLinearColor(1.f, 1.f, 1.f, 0.15f), 5.f));
		KeyBox->SetVisibility(Reason == ENOARInteractDenyReason::TooFar && CameraMode == ENOARCameraMode::ThirdPerson ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (PromptKey) PromptKey->SetText(FText::FromString(CameraMode == ENOARCameraMode::ThirdPerson ? TEXT("CLICK") : TEXT("E")));
}

void UNOARHUDWidget::HandleRoomEntered(UNOARRoomData* Room)
{
	if (Room) ShowToast(Room->DisplayName, 2.5f);
}

void UNOARHUDWidget::HandleCameraMode(ENOARCameraMode Mode)
{
	CameraMode = Mode;
	UpdateHints();
}

void UNOARHUDWidget::ShowToast(const FText& Text, float Seconds)
{
	if (!Toast || !ToastText) return;
	ToastText->SetText(Text);
	ToastTimer = Seconds;
	Toast->SetRenderOpacity(1.f);
}

void UNOARHUDWidget::UpdateHints()
{
	if (!HintsText) return;
	HintsText->SetText(CameraMode == ENOARCameraMode::FirstPerson
		? LOCTEXT("HintsFP", "WASD walk  ·  trackpad / arrows look  ·  E open / close  ·  Shift fast  ·  V third person")
		: LOCTEXT("Hints3P", "click floor to walk  ·  click a door or drawer  ·  WASD walk  ·  arrows turn  ·  V first person"));
}

#undef LOCTEXT_NAMESPACE
