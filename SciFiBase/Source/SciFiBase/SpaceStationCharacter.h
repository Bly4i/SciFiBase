#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DamageableInterface.h"
#include "SpaceStationCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class SCIFIBASE_API ASpaceStationCharacter : public ACharacter, public IDamageableInterface
{
	GENERATED_BODY()

public:
	ASpaceStationCharacter();

protected:
	virtual void BeginPlay() override;

	// Компоненты камеры
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// Характеристики персонажа
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Stamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bIsDead;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Реализация интерфейса урона (BlueprintNativeEvent генерирует суффикс _Implementation)
	virtual void TakeCustomDamage_Implementation(float DamageAmount, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Die();
};