#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BochaMinimapWidget.generated.h"

/**
 * Schematic minimap in the top right corner: a high-contrast stretch of the court with its lines
 * and the balls drawn as enlarged dots. Shows WindowLength of the court around the jack (or the
 * jack's landing zone before it is thrown), far end on top, as seen from the throwing end.
 */
UCLASS()
class BOCHANET_API UBochaMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/** Length of court shown (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float WindowLength = 1000.f;

	/** Height of the map on screen; the width follows the court's proportions. */
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float MapHeight = 440.f;

	/** Distance from the top right corner of the screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FVector2D ScreenMargin = FVector2D(24.0, 24.0);

	/** Balls are drawn this many times their real size... */
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float BallScale = 2.f;

	/** ...and never smaller than this (px). */
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	float MinBallSize = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.05f, 0.04f, 0.85f);

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FLinearColor RedColor = FLinearColor(0.95f, 0.15f, 0.1f);

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FLinearColor BlueColor = FLinearColor(0.15f, 0.45f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	FLinearColor BolimColor = FLinearColor(1.f, 0.9f, 0.1f);
};
