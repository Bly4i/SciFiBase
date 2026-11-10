
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UDamageableInterface : public UInterface
{
	GENERATED_BODY()
};


class SCIFIBASE_API IDamageableInterface
{
	GENERATED_BODY()

public:
	// Функция получения урона, которую можно переопределить и в C++, и в Blueprints
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void TakeCustomDamage(float DamageAmount, AActor* DamageCauser);
};
