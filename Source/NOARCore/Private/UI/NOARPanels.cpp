#include "UI/NOARPanels.h"
#include "UI/NOARUIStyle.h"
#include "Core/NOARPlayerController.h"
#include "Core/NOARGameInstance.h"
#include "Core/NOARApartmentSubsystem.h"
#include "Configurator/NOARConfiguratorSubsystem.h"
#include "Lighting/NOARLightingSubsystem.h"
#include "Lighting/NOARDayNightManager.h"
#include "Character/NOARCharacter.h"
#include "Data/NOARDataAssets.h"
#include "NOARSettings.h"
#include "NOARFunctionLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "NOARUI"

static FName SlotOptionKey(const FGameplayTag& Slot, FName Option)
{
	return FName(*(Slot.ToString() + TEXT("|") + Option.ToString()));
}

// ============================================================================ Finishes

UNOARFinishesPanel::UNOARFinishesPanel(const FObjectInitializer& OI) : Super(OI)
{
	PanelName = TEXT("Finishes");
	Title = LOCTEXT("Finishes", "Finishes");
	CardWidth = 560.f;
	bDockRight = true;
}

void UNOARFinishesPanel::BuildContent()
{
	UHorizontalBox* Top = MakeRow();
	UTextBlock* Hint = MakeText(LOCTEXT("FinishesHint", "Pick a finish - it applies instantly and is saved with your design."), 12, NOARUI::TextDim());
	UHorizontalBoxSlot* HS = Top->AddChildToHorizontalBox(Hint);
	HS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HS->SetVerticalAlignment(VAlign_Center);
	UNOARIdButton* Reset = MakeButton(LOCTEXT("ResetFinishes", "Reset to design"), TEXT("Reset"), FGameplayTag(), false, 12);
	Reset->OnPressedId.AddDynamic(this, &UNOARFinishesPanel::HandleReset);
	UHorizontalBoxSlot* RS = Top->AddChildToHorizontalBox(Reset);
	RS->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	Content->AddChildToVerticalBox(Top);

	PriceText = MakeText(FText::GetEmpty(), 13, NOARUI::Accent(), TEXT("Bold"));
	UVerticalBoxSlot* PS = Content->AddChildToVerticalBox(PriceText);
	PS->SetPadding(FMargin(2.f, 8.f, 0.f, 0.f));
	PriceText->SetVisibility(ESlateVisibility::Collapsed);
}

FLinearColor UNOARFinishesPanel::SwatchColorFor(const FNOARFinishOptionRow& Row) const
{
	if (UMaterialInterface* M = Row.Material.LoadSynchronous())
	{
		FLinearColor C;
		if (M->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Tint")), C))
		{
			return FLinearColor(C.R, C.G, C.B, 1.f);
		}
	}
	return FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
}

void UNOARFinishesPanel::RebuildList()
{
	UNOARConfiguratorSubsystem* Cfg = GetWorld() ? GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>() : nullptr;
	if (!Cfg) return;
	// drop old sections (everything after the first two fixed children)
	while (Content->GetChildrenCount() > 2)
	{
		Content->RemoveChildAt(Content->GetChildrenCount() - 1);
	}
	OptionButtons.Empty();

	TArray<FGameplayTag> Slots = Cfg->GetSlots();
	if (Slots.Num() == 0)
	{
		Content->AddChildToVerticalBox(MakeText(LOCTEXT("NoSlots", "No configurable surfaces in this apartment yet."), 13, NOARUI::TextDim()));
	}
	for (const FGameplayTag& SlotTag : Slots)
	{
		UBorder* Section = MakeSection(NOARUI::LabelFromTag(SlotTag));
		UWrapBox* Wrap = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
		Wrap->SetInnerSlotPadding(FVector2D(6.f, 6.f));
		Section->SetContent(Wrap);
		const FName CurrentOpt = Cfg->GetCurrentOption(SlotTag);
		for (const FNOARFinishOptionRow& Row : Cfg->GetOptionsForSlot(SlotTag))
		{
			UNOARIdButton* B = WidgetTree->ConstructWidget<UNOARIdButton>(UNOARIdButton::StaticClass());
			B->Id = Row.OptionID;
			B->Tag = SlotTag;
			B->OnPressedId.AddDynamic(this, &UNOARFinishesPanel::HandleOption);
			UHorizontalBox* Inner = MakeRow();
			UImage* Sw = MakeSwatch(SwatchColorFor(Row), 26.f);
			UHorizontalBoxSlot* SS = Inner->AddChildToHorizontalBox(Sw);
			SS->SetVerticalAlignment(VAlign_Center);
			SS->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			UVerticalBox* Labels = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			UTextBlock* Name = MakeText(Row.DisplayName, 13, NOARUI::Text());
			Name->SetAutoWrapText(false);
			Labels->AddChildToVerticalBox(Name);
			FString Sub = Row.SpecCode;
			if (!Row.Manufacturer.IsEmpty()) Sub += TEXT(" · ") + Row.Manufacturer;
			if (GetDefault<UNOARSettings>()->bShowPrices && Row.Price > 0.f)
			{
				Sub += FString::Printf(TEXT(" · $%.0f%s"), Row.Price, Row.PriceUnit.IsEmpty() ? TEXT("") : *(TEXT("/") + Row.PriceUnit));
			}
			UTextBlock* SubT = MakeText(FText::FromString(Sub), 10, NOARUI::TextDim());
			SubT->SetAutoWrapText(false);
			Labels->AddChildToVerticalBox(SubT);
			UHorizontalBoxSlot* LS = Inner->AddChildToHorizontalBox(Labels);
			LS->SetVerticalAlignment(VAlign_Center);
			B->SetContent(Inner);
			B->SetSelected(Row.OptionID == CurrentOpt);
			Wrap->AddChildToWrapBox(B);
			OptionButtons.Add(SlotOptionKey(SlotTag, Row.OptionID), B);
		}
	}
	bListBuilt = true;
}

void UNOARFinishesPanel::Refresh_Implementation()
{
	UNOARConfiguratorSubsystem* Cfg = GetWorld() ? GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>() : nullptr;
	if (Cfg && !Cfg->OnConfigChanged.IsAlreadyBound(this, &UNOARFinishesPanel::HandleConfigChanged))
	{
		Cfg->OnConfigChanged.AddDynamic(this, &UNOARFinishesPanel::HandleConfigChanged);
	}
	RebuildList();
	if (PriceText && Cfg)
	{
		const bool bPrices = GetDefault<UNOARSettings>()->bShowPrices;
		PriceText->SetVisibility(bPrices ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bPrices) PriceText->SetText(FText::Format(LOCTEXT("PriceTotal", "Selected upgrades: ${0}"), FText::AsNumber(FMath::RoundToInt(Cfg->GetCurrentPriceTotal()))));
	}
}

void UNOARFinishesPanel::HandleOption(FName Id, FGameplayTag Tag)
{
	if (UNOARConfiguratorSubsystem* Cfg = GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>())
	{
		Cfg->ApplyOption(Tag, Id, true);
	}
}

void UNOARFinishesPanel::HandleReset(FName, FGameplayTag)
{
	if (UNOARConfiguratorSubsystem* Cfg = GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>())
	{
		Cfg->ResetToDefaults();
		Refresh();
	}
}

void UNOARFinishesPanel::HandleConfigChanged(FGameplayTag ChangedSlot, FName OptionID)
{
	for (auto& Pair : OptionButtons)
	{
		if (Pair.Value && Pair.Value->Tag == ChangedSlot)
		{
			Pair.Value->SetSelected(Pair.Value->Id == OptionID);
		}
	}
	if (PriceText && GetDefault<UNOARSettings>()->bShowPrices)
	{
		if (UNOARConfiguratorSubsystem* Cfg = GetWorld()->GetSubsystem<UNOARConfiguratorSubsystem>())
		{
			PriceText->SetText(FText::Format(LOCTEXT("PriceTotal", "Selected upgrades: ${0}"), FText::AsNumber(FMath::RoundToInt(Cfg->GetCurrentPriceTotal()))));
		}
	}
}

// ============================================================================ Rooms

UNOARRoomsPanel::UNOARRoomsPanel(const FObjectInitializer& OI) : Super(OI)
{
	PanelName = TEXT("Rooms");
	Title = LOCTEXT("Rooms", "Rooms");
	CardWidth = 360.f;
	bDockRight = true;
}

void UNOARRoomsPanel::Refresh_Implementation()
{
	Content->ClearChildren();
	UNOARApartmentSubsystem* Apt = GetWorld() ? GetWorld()->GetSubsystem<UNOARApartmentSubsystem>() : nullptr;
	if (!Apt || Apt->GetRooms().Num() == 0)
	{
		Content->AddChildToVerticalBox(MakeText(LOCTEXT("NoRooms", "No rooms defined in DA_Apartment."), 13, NOARUI::TextDim()));
		return;
	}
	UNOARRoomData* Cur = Apt->GetCurrentRoom();
	for (UNOARRoomData* R : Apt->GetRooms())
	{
		if (!R) continue;
		UNOARIdButton* B = MakeButton(R->DisplayName, R->RoomID, R->ZoneTag, false, 15);
		B->SetSelected(R == Cur);
		B->OnPressedId.AddDynamic(this, &UNOARRoomsPanel::HandleRoom);
		UVerticalBoxSlot* S = Content->AddChildToVerticalBox(B);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		S->SetHorizontalAlignment(HAlign_Fill);
	}
}

void UNOARRoomsPanel::HandleRoom(FName Id, FGameplayTag)
{
	if (UNOARApartmentSubsystem* Apt = GetWorld()->GetSubsystem<UNOARApartmentSubsystem>())
	{
		Apt->TeleportToRoom(Id);
	}
	Close();
}

// ============================================================================ Lighting

UNOARLightingPanel::UNOARLightingPanel(const FObjectInitializer& OI) : Super(OI)
{
	PanelName = TEXT("Lighting");
	Title = LOCTEXT("Lighting", "Light");
	CardWidth = 400.f;
	bDockRight = true;
}

ANOARDayNightManager* UNOARLightingPanel::FindDayNight() const
{
	if (UWorld* W = GetWorld())
	{
		TActorIterator<ANOARDayNightManager> It(W);
		if (It) return *It;
	}
	return nullptr;
}

void UNOARLightingPanel::BuildContent()
{
	UBorder* Sec = MakeSection(LOCTEXT("TimeOfDay", "TIME OF DAY"));
	UHorizontalBox* Row = MakeRow();
	Sec->SetContent(Row);
	for (const auto& P : { TPair<FName, FText>(TEXT("Day"), LOCTEXT("Day", "Day")), TPair<FName, FText>(TEXT("Evening"), LOCTEXT("Evening", "Evening")), TPair<FName, FText>(TEXT("Night"), LOCTEXT("Night", "Night")) })
	{
		UNOARIdButton* B = MakeButton(P.Value, P.Key);
		B->OnPressedId.AddDynamic(this, &UNOARLightingPanel::HandlePreset);
		UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(B);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		PresetButtons.Add(P.Key, B);
	}
	UBorder* GroupsSec = MakeSection(LOCTEXT("LightGroups", "LIGHTS"));
	GroupList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	GroupsSec->SetContent(GroupList);
}

void UNOARLightingPanel::Refresh_Implementation()
{
	ANOARDayNightManager* DN = FindDayNight();
	const ENOARLightingPreset Cur = DN ? DN->GetCurrentPreset() : ENOARLightingPreset::Day;
	for (auto& P : PresetButtons)
	{
		const bool bSel = (P.Key == TEXT("Day") && Cur == ENOARLightingPreset::Day) || (P.Key == TEXT("Evening") && Cur == ENOARLightingPreset::Evening) || (P.Key == TEXT("Night") && Cur == ENOARLightingPreset::Night);
		P.Value->SetSelected(bSel);
	}
	UNOARLightingSubsystem* L = GetWorld() ? GetWorld()->GetSubsystem<UNOARLightingSubsystem>() : nullptr;
	if (!L || !GroupList) return;
	if (!L->OnGroupChanged.IsAlreadyBound(this, &UNOARLightingPanel::HandleGroupChanged))
	{
		L->OnGroupChanged.AddDynamic(this, &UNOARLightingPanel::HandleGroupChanged);
	}
	GroupList->ClearChildren();
	GroupDots.Empty();
	TArray<FGameplayTag> Groups = L->GetKnownGroups();
	Groups.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
	if (Groups.Num() == 0)
	{
		GroupList->AddChildToVerticalBox(MakeText(LOCTEXT("NoGroups", "No interactive lights registered."), 12, NOARUI::TextDim()));
	}
	for (const FGameplayTag& G : Groups)
	{
		UNOARIdButton* B = WidgetTree->ConstructWidget<UNOARIdButton>(UNOARIdButton::StaticClass());
		B->Tag = G;
		B->OnPressedId.AddDynamic(this, &UNOARLightingPanel::HandleGroup);
		UHorizontalBox* Inner = MakeRow();
		UTextBlock* T = MakeText(NOARUI::LabelFromTag(G), 13, NOARUI::Text());
		T->SetAutoWrapText(false);
		UHorizontalBoxSlot* TS = Inner->AddChildToHorizontalBox(T);
		TS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TS->SetVerticalAlignment(VAlign_Center);
		const bool bOn = L->IsGroupOn(G);
		UImage* Dot = MakeSwatch(bOn ? NOARUI::Accent() : FLinearColor(1.f, 1.f, 1.f, 0.12f), 12.f);
		UHorizontalBoxSlot* DS = Inner->AddChildToHorizontalBox(Dot);
		DS->SetVerticalAlignment(VAlign_Center);
		B->SetContent(Inner);
		UVerticalBoxSlot* S = GroupList->AddChildToVerticalBox(B);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		S->SetHorizontalAlignment(HAlign_Fill);
		GroupDots.Add(G, Dot);
	}
}

void UNOARLightingPanel::HandlePreset(FName Id, FGameplayTag)
{
	if (ANOARDayNightManager* DN = FindDayNight())
	{
		DN->SetPreset(Id == TEXT("Day") ? ENOARLightingPreset::Day : Id == TEXT("Evening") ? ENOARLightingPreset::Evening : ENOARLightingPreset::Night);
	}
	Refresh();
}

void UNOARLightingPanel::HandleGroup(FName, FGameplayTag Tag)
{
	if (UNOARLightingSubsystem* L = GetWorld()->GetSubsystem<UNOARLightingSubsystem>())
	{
		L->ToggleGroup(Tag);
	}
}

void UNOARLightingPanel::HandleGroupChanged(FGameplayTag Group, bool bOn, float)
{
	if (const TObjectPtr<UImage>* Dot = GroupDots.Find(Group))
	{
		if (*Dot)
		{
			FSlateBrush B = NOARUI::Box(bOn ? NOARUI::Accent() : FLinearColor(1.f, 1.f, 1.f, 0.12f), 6.f, FLinearColor(1.f, 1.f, 1.f, 0.15f), 1.f);
			B.ImageSize = FVector2D(12.f, 12.f);
			(*Dot)->SetBrush(B);
		}
	}
}

// ============================================================================ Menu

UNOARMenuPanel::UNOARMenuPanel(const FObjectInitializer& OI) : Super(OI)
{
	PanelName = TEXT("Menu");
	Title = LOCTEXT("Menu", "Menu");
	CardWidth = 420.f;
	bDockRight = false;
}

void UNOARMenuPanel::BuildContent()
{
	auto AddAction = [this](const FText& Label, FName Id, bool bAccent = false) -> UNOARIdButton*
	{
		UNOARIdButton* B = MakeButton(Label, Id, FGameplayTag(), bAccent, 15);
		B->OnPressedId.AddDynamic(this, &UNOARMenuPanel::HandleAction);
		UVerticalBoxSlot* S = Content->AddChildToVerticalBox(B);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		S->SetHorizontalAlignment(HAlign_Fill);
		return B;
	};
	AddAction(LOCTEXT("Resume", "Resume"), TEXT("Resume"), true);
	CameraButton = AddAction(LOCTEXT("Camera", "Camera"), TEXT("Camera"));
	AddAction(LOCTEXT("ResetDesign", "Reset my design"), TEXT("Reset"));

	UBorder* Q = MakeSection(LOCTEXT("Quality", "QUALITY"));
	UHorizontalBox* Row = MakeRow();
	Q->SetContent(Row);
	for (const auto& P : { TPair<FName, FText>(TEXT("Low"), LOCTEXT("Low", "Low")), TPair<FName, FText>(TEXT("Medium"), LOCTEXT("Medium", "Medium")), TPair<FName, FText>(TEXT("High"), LOCTEXT("High", "High")), TPair<FName, FText>(TEXT("Ultra"), LOCTEXT("Ultra", "Ultra")) })
	{
		UNOARIdButton* B = MakeButton(P.Value, P.Key, FGameplayTag(), false, 13);
		B->OnPressedId.AddDynamic(this, &UNOARMenuPanel::HandleAction);
		UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(B);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		QualityButtons.Add(P.Key, B);
	}
	UTextBlock* Hint = MakeText(LOCTEXT("QualityHint", "Ultra enables hardware ray tracing on RTX GPUs. Not available on Mac."), 11, NOARUI::TextDim());
	UVerticalBoxSlot* HS = Content->AddChildToVerticalBox(Hint);
	HS->SetPadding(FMargin(2.f, 4.f, 0.f, 10.f));

	UNOARIdButton* Quit = AddAction(LOCTEXT("Quit", "Quit"), TEXT("Quit"));
	Quit->SetStyle(NOARUI::ButtonStyle(FLinearColor(0.75f, 0.30f, 0.25f, 0.18f), FLinearColor(0.75f, 0.30f, 0.25f, 0.32f), FLinearColor(0.75f, 0.30f, 0.25f, 0.45f)));
}

void UNOARMenuPanel::Refresh_Implementation()
{
	UNOARGameInstance* GI = GetGameInstance<UNOARGameInstance>();
	const ENOARQualityTier Cur = GI ? GI->GetQualityTier() : ENOARQualityTier::High;
	for (auto& P : QualityButtons)
	{
		const ENOARQualityTier T = P.Key == TEXT("Low") ? ENOARQualityTier::Low : P.Key == TEXT("Medium") ? ENOARQualityTier::Medium : P.Key == TEXT("High") ? ENOARQualityTier::High : ENOARQualityTier::Ultra;
		P.Value->SetSelected(T == Cur);
		P.Value->SetIsEnabled(!(T == ENOARQualityTier::Ultra && UNOARFunctionLibrary::IsMac()));
	}
	if (CameraButton)
	{
		ANOARPlayerController* PC = GetNOARController();
		const bool bFP = PC && PC->GetNOARCharacter() && PC->GetNOARCharacter()->GetCameraMode() == ENOARCameraMode::FirstPerson;
		if (UTextBlock* T = Cast<UTextBlock>(CameraButton->GetContent()))
		{
			T->SetText(bFP ? LOCTEXT("CameraFP", "Camera: first person  (V)") : LOCTEXT("Camera3P", "Camera: third person  (V)"));
		}
	}
}

void UNOARMenuPanel::HandleAction(FName Id, FGameplayTag)
{
	ANOARPlayerController* PC = GetNOARController();
	if (Id == TEXT("Resume"))
	{
		Close();
	}
	else if (Id == TEXT("Camera"))
	{
		if (PC && PC->GetNOARCharacter()) PC->GetNOARCharacter()->ToggleCameraMode();
		Refresh();
	}
	else if (Id == TEXT("Reset"))
	{
		if (UNOARApartmentSubsystem* Apt = GetWorld()->GetSubsystem<UNOARApartmentSubsystem>()) Apt->ResetDesign();
		Close();
	}
	else if (Id == TEXT("Quit"))
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
	else if (UNOARGameInstance* GI = GetGameInstance<UNOARGameInstance>())
	{
		const ENOARQualityTier T = Id == TEXT("Low") ? ENOARQualityTier::Low : Id == TEXT("Medium") ? ENOARQualityTier::Medium : Id == TEXT("High") ? ENOARQualityTier::High : ENOARQualityTier::Ultra;
		GI->SetQualityTier(T, T == ENOARQualityTier::Ultra && UNOARFunctionLibrary::IsHardwareRayTracingAvailable());
		Refresh();
	}
}

// ============================================================================ Product

UNOARProductPanel::UNOARProductPanel(const FObjectInitializer& OI) : Super(OI)
{
	PanelName = TEXT("Product");
	Title = LOCTEXT("Product", "Product");
	CardWidth = 380.f;
	bDockRight = true;
}

void UNOARProductPanel::BuildContent()
{
	Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(Rows);
}

void UNOARProductPanel::SetProduct(const FNOARProductInfo& Info)
{
	Current = Info;
	if (IsOpen()) Refresh();
}

void UNOARProductPanel::Refresh_Implementation()
{
	if (!Rows) return;
	Rows->ClearChildren();
	auto AddRow = [this](const FText& Label, const FString& Value)
	{
		if (Value.IsEmpty()) return;
		UHorizontalBox* R = MakeRow();
		UTextBlock* L = MakeText(Label, 11, NOARUI::TextDim(), TEXT("Bold"));
		L->SetAutoWrapText(false);
		UHorizontalBoxSlot* LS = R->AddChildToHorizontalBox(L);
		LS->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		LS->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		LS->SetVerticalAlignment(VAlign_Top);
		UTextBlock* V = MakeText(FText::FromString(Value), 13, NOARUI::Text());
		UHorizontalBoxSlot* VS = R->AddChildToHorizontalBox(V);
		VS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UVerticalBoxSlot* S = Rows->AddChildToVerticalBox(R);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	};
	if (TitleText) TitleText->SetText(Current.SpecCode.IsEmpty() ? LOCTEXT("Product", "Product") : FText::FromString(Current.SpecCode));
	AddRow(LOCTEXT("Manufacturer", "MAKER"), Current.Manufacturer);
	AddRow(LOCTEXT("Model", "MODEL"), Current.Model);
	AddRow(LOCTEXT("Material", "MATERIAL"), Current.Material);
	AddRow(LOCTEXT("Finish", "FINISH"), Current.Finish);
	AddRow(LOCTEXT("Color", "COLOUR"), Current.Color);
	if (GetDefault<UNOARSettings>()->bShowPrices && Current.Price > 0.f)
	{
		AddRow(LOCTEXT("Price", "PRICE"), FString::Printf(TEXT("$%.0f"), Current.Price));
	}
	if (Rows->GetChildrenCount() == 0)
	{
		Rows->AddChildToVerticalBox(MakeText(LOCTEXT("NoProduct", "No product data for this item."), 13, NOARUI::TextDim()));
	}
}

// ============================================================================ Photo overlay

UNOARPhotoOverlay::UNOARPhotoOverlay(const FObjectInitializer& OI) : Super(OI)
{
	PanelName = TEXT("Photo");
	Title = LOCTEXT("Photo", "Photo");
	bModal = false;
	CardWidth = 0.f;
	bDockRight = false;
	OpenVisibility = ESlateVisibility::HitTestInvisible;
}

TSharedRef<SWidget> UNOARPhotoOverlay::RebuildWidget()
{
	if (!bBuilt && WidgetTree)
	{
		bBuilt = true;
		Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		UBorder* Pill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Pill"));
		Pill->SetBrush(NOARUI::Box(NOARUI::Panel(), 18.f, NOARUI::PanelLine(), 1.f));
		Pill->SetPadding(FMargin(16.f, 8.f));
		UTextBlock* T = MakeText(LOCTEXT("PhotoHint", "Photo mode — WASD / mouse to frame, P or Esc to exit"), 12, NOARUI::TextDim());
		T->SetAutoWrapText(false);
		Pill->SetContent(T);
		UOverlaySlot* S = Root->AddChildToOverlay(Pill);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Bottom);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, 28.f));
		SetRenderOpacity(0.f);
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return UUserWidget::RebuildWidget();
}

#undef LOCTEXT_NAMESPACE
