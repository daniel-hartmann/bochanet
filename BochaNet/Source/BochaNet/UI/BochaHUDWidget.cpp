#include "UI/BochaHUDWidget.h"

#include "Components/Image.h"
#include "Components/Slider.h"

float UBochaHUDWidget::GetThrowStrength() const
{
	return Slider_Strength->GetValue();
}

void UBochaHUDWidget::SetThrowStrength(float Strength)
{
	Slider_Strength->SetValue(FMath::Clamp(Strength, MinThrowStrength, MaxThrowStrength));
}

void UBochaHUDWidget::SetP1Bolim()
{
	P1_Bolim->SetVisibility(ESlateVisibility::Visible);
	P2_Bolim->SetVisibility(ESlateVisibility::Hidden);
}
