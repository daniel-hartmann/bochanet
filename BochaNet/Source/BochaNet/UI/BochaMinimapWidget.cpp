#include "UI/BochaMinimapWidget.h"

#include "Actors/BochaBall.h"
#include "Actors/BochaBocha.h"
#include "Actors/BochaBolim.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "EngineUtils.h"
#include "Game/BochaMatch.h"
#include "Rendering/DrawElements.h"

namespace
{
	const FSlateColorBrush& SolidBrush()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return Brush;
	}

	// Without a radius the brush rounds by half its height, so a square drawn with it is a circle.
	// (A fixed radius larger than the box draws nothing.)
	const FSlateRoundedBoxBrush& CircleBrush()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return Brush;
	}

	void DrawRect(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2f& Position, const FVector2f& Size, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
			&SolidBrush(), ESlateDrawEffect::None, Color);
	}

	void DrawCircle(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
		const FVector2f& Center, float Diameter, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(Elements, Layer,
			Geometry.ToPaintGeometry(FVector2f(Diameter), FSlateLayoutTransform(Center - FVector2f(Diameter * 0.5f))),
			&CircleBrush(), ESlateDrawEffect::None, Color);
	}
}

void UBochaMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Drawn over the game; never takes the mouse.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// The balls already in play, once; from now on the balls' events keep the list.
	for (TActorIterator<ABochaBall> It(GetWorld()); It; ++It)
	{
		AddBall(*It);
	}
	ABochaBall::OnBallBeginPlay.AddUObject(this, &UBochaMinimapWidget::AddBall);
	ABochaBall::OnBallEndPlay.AddUObject(this, &UBochaMinimapWidget::RemoveBall);
}

void UBochaMinimapWidget::NativeDestruct()
{
	ABochaBall::OnBallBeginPlay.RemoveAll(this);
	ABochaBall::OnBallEndPlay.RemoveAll(this);
	Balls.Reset();
	Super::NativeDestruct();
}

void UBochaMinimapWidget::AddBall(ABochaBall* Ball)
{
	// The delegates are shared by every world (several PIE players run in one process).
	if (Ball && Ball->GetWorld() == GetWorld())
	{
		Balls.AddUnique(Ball);
	}
}

void UBochaMinimapWidget::RemoveBall(ABochaBall* Ball)
{
	Balls.Remove(Ball);
}

int32 UBochaMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!Match.IsValid())
	{
		Match = ABochaMatch::Find(this);
	}
	if (!Match.IsValid())
	{
		return LayerId;
	}
	const FBochaCourt& Court = Match->GetCourt();

	// Thrown balls only: the one in the player's hand is attached to the character.
	const ABochaBall* Bolim = nullptr;
	TArray<const ABochaBall*> InPlay;
	for (const TWeakObjectPtr<ABochaBall>& WeakBall : Balls)
	{
		const ABochaBall* Ball = WeakBall.Get();
		if (!Ball || Ball->GetAttachParentActor())
		{
			continue;
		}
		InPlay.Add(Ball);
		if (Ball->IsA<ABochaBolim>())
		{
			Bolim = Ball;
		}
	}

	// Window along the court: around the jack, or over its landing zone (E..B') before it is thrown.
	const double Center = Bolim ? Match->ToCourt(Bolim->GetActorLocation()).X : (Court.E + Court.BPrime) * 0.5;
	const double WindowMin = FMath::Clamp(Center - WindowLength * 0.5, Court.A, Court.APrime - WindowLength);
	const double WindowMax = WindowMin + WindowLength;

	const double Width = Court.SideMax - Court.SideMin;
	const float Scale = MapHeight / WindowLength;
	const FVector2f MapSize(Width * Scale, MapHeight);
	const FVector2f MapPosition(AllottedGeometry.GetLocalSize().X - MapSize.X - ScreenMargin.X, ScreenMargin.Y);

	// Court X runs up the map (far end on top); court Y runs left to right, as seen from the throwing end.
	auto ToMap = [&](double X, double Y)
	{
		return MapPosition + FVector2f((Y - Court.SideMin) * Scale, (WindowMax - X) * Scale);
	};

	const int32 BackgroundLayer = LayerId + 1;
	const int32 LineLayer = LayerId + 2;
	const int32 BallLayer = LayerId + 3;

	// Frame and background.
	DrawRect(OutDrawElements, BackgroundLayer, AllottedGeometry, MapPosition - FVector2f(2.f), MapSize + FVector2f(4.f), FLinearColor(1.f, 1.f, 1.f, 0.9f));
	DrawRect(OutDrawElements, BackgroundLayer, AllottedGeometry, MapPosition, MapSize, BackgroundColor);

	// Court lines inside the window, mirrored ones measured from A'.
	auto DrawLine = [&](double X, const FLinearColor& Color, float Thickness)
	{
		if (X < WindowMin || X > WindowMax)
		{
			return;
		}
		const TArray<FVector2f> Points = { ToMap(X, Court.SideMin), ToMap(X, Court.SideMax) };
		FSlateDrawElement::MakeLines(OutDrawElements, LineLayer, AllottedGeometry.ToPaintGeometry(), Points,
			ESlateDrawEffect::None, Color, true, Thickness);
	};
	const double Mirror = Court.A + Court.APrime;
	DrawLine(Court.E, FLinearColor(1.f, 0.1f, 0.1f), 2.f);
	DrawLine(Mirror - Court.D, FLinearColor(1.f, 0.85f, 0.1f), 1.5f);
	DrawLine(Mirror - Court.C, FLinearColor(0.2f, 0.5f, 1.f), 1.5f);
	DrawLine(Court.BPrime, FLinearColor(0.1f, 0.9f, 0.2f), 2.f);
	DrawLine(Court.APrime, FLinearColor::White, 4.f);

	// Balls: dark outline, then the color; the jack last so it stays visible among the bochas.
	InPlay.Sort([](const ABochaBall& A, const ABochaBall& B) { return !A.IsA<ABochaBolim>() && B.IsA<ABochaBolim>(); });
	for (const ABochaBall* Ball : InPlay)
	{
		const FVector Location = Match->ToCourt(Ball->GetActorLocation());
		if (Location.X < WindowMin || Location.X > WindowMax)
		{
			continue;
		}
		const ABochaBocha* Bocha = Cast<ABochaBocha>(Ball);
		const FLinearColor Color = !Bocha ? BolimColor : Bocha->bBlue ? BlueColor : RedColor;
		const float Diameter = FMath::Max(MinBallSize, 2.f * Ball->GetRadius() * Scale * BallScale);
		const FVector2f Position = ToMap(Location.X, Location.Y);
		DrawCircle(OutDrawElements, BallLayer, AllottedGeometry, Position, Diameter + 3.f, FLinearColor(0.f, 0.f, 0.f, 0.9f));
		DrawCircle(OutDrawElements, BallLayer, AllottedGeometry, Position, Diameter, Color);
	}

	return BallLayer;
}
